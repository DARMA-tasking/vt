
#include "vt/config.h"
#include "vt/runtime/runtime.h"
#include "vt/context/context.h"
#include "vt/context/context_attorney.h"
#include "vt/registry/registry.h"
#include "vt/messaging/active.h"
#include "vt/event/event.h"
#include "vt/termination/termination.h"
#include "vt/pool/pool.h"
#include "vt/rdma/rdma_headers.h"
#include "vt/parameterization/parameterization.h"
#include "vt/sequence/sequencer_headers.h"
#include "vt/trace/trace.h"
#include "vt/scheduler/scheduler.h"
#include "vt/topos/location/location_headers.h"
#include "vt/vrt/context/context_vrtmanager.h"
#include "vt/vrt/collection/collection_headers.h"
#include "vt/worker/worker_headers.h"
#include "vt/configs/generated/vt_git_revision.h"
#include "vt/configs/debug/debug_colorize.h"
#include "vt/configs/arguments/args.h"
#include "vt/configs/error/stack_out.h"
#include "vt/configs/error/pretty_print_stack.h"

#include <memory>
#include <functional>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <limits.h>
#include <unistd.h>
#include <csignal>

#include <fmt/color.h>

namespace vt { namespace runtime {

/*static*/ std::string Runtime::prog_name_ = "";

Runtime::Runtime(
  int& argc, char**& argv, WorkerCountType in_num_workers,
  bool const interop_mode, MPI_Comm* in_comm, RuntimeInstType const in_instance
)  : instance_(in_instance), runtime_active_(false), is_interop_(interop_mode),
     num_workers_(in_num_workers), communicator_(in_comm), user_argc_(argc),
     user_argv_(argv)
{
  ArgType::parse(argc, argv);
  if (argc > 0) {
    prog_name_ = std::string(argv[0]);
  }
  setupSignalHandler();
  setupSignalHandlerINT();
  setupTerminateHandler();
}

/*static*/ void Runtime::sigHandlerINT(int sig) {
  auto node      = vt::theContext() ? vt::theContext()->getNode() : -1;
  auto vt_pre    = debug::vtPre();
  auto node_str  = ::vt::debug::proc(node);
  auto prefix    = vt_pre + node_str + " ";
  if (node == 0 || node == -1) {
    ::fmt::print("{}Caught SIGINT signal: {} \n", prefix, sig);
  }
  if (Runtime::nodeStackWrite()) {
    auto stack = debug::stack::dumpStack();
    auto stack_pretty = debug::stack::prettyPrintStack(std::get<1>(stack));
    if (ArgType::vt_stack_file != "") {
      Runtime::writeToFile(stack_pretty);
    } else {
      ::fmt::print("{}", stack_pretty);
      ::fmt::print("\n");
    }
  }
  std::exit(1);
}

/*static*/ void Runtime::sigHandler(int sig) {
  auto vt_pre    = debug::vtPre();
  auto bred      = debug::bred();
  ::fmt::print("{}Caught SIGSEGV signal: {} \n", vt_pre, sig);
  if (Runtime::nodeStackWrite()) {
    auto stack = debug::stack::dumpStack();
    if (ArgType::vt_stack_file != "") {
      Runtime::writeToFile(std::get<0>(stack));
    } else {
      ::fmt::print("{}{}{}\n", bred, std::get<0>(stack), debug::reset());
      ::fmt::print("\n");
    }
  }
  std::exit(1);
}

/*static*/ void Runtime::termHandler() {
  auto vt_pre    = debug::vtPre();
  auto bred      = debug::bred();
  ::fmt::print("{}Caught std::terminate \n", vt_pre);
  if (Runtime::nodeStackWrite()) {
    auto stack = debug::stack::dumpStack();
    if (ArgType::vt_stack_file != "") {
      Runtime::writeToFile(std::get<0>(stack));
    } else {
      ::fmt::print("{}{}{}\n", bred, std::get<0>(stack), debug::reset());
      ::fmt::print("\n");
    }
  }
  std::exit(1);
}

/*static*/ bool Runtime::nodeStackWrite() {
  auto const& node = debug::preNode();
  if (node == uninitialized_destination) {
    return true;
  } else if (ArgType::vt_stack_mod == 0) {
    return true;
  } else if (node % ArgType::vt_stack_mod == 0) {
    return true;
  } else {
    return false;
  }
}

/*static*/ void Runtime::writeToFile(std::string const& str) {
  std::string app_name = prog_name_ == "" ? "prog" : prog_name_;
  std::string name = ArgType::vt_stack_file == "" ? app_name : ArgType::vt_stack_file;
  auto const& node = debug::preNode();
  std::string file = name + "." + std::to_string(node) + ".stack.out";
  std::string dir  = ArgType::vt_stack_dir == "" ? "" : ArgType::vt_stack_dir + "/";
  std::string path = dir + file;
  FILE* f = fopen(path.c_str(), "w+");
  fprintf(f, "%s", str.c_str());
  fclose(f);
}

void Runtime::setupSignalHandler() {
  if (!ArgType::vt_no_sigsegv) {
    signal(SIGSEGV, Runtime::sigHandler);
  }
}

void Runtime::setupSignalHandlerINT() {
  if (!ArgType::vt_no_sigint) {
    signal(SIGINT, Runtime::sigHandlerINT);
  }
}

void Runtime::setupTerminateHandler() {
  if (!ArgType::vt_no_terminate) {
    std::set_terminate(termHandler);
  }
}

/*virtual*/ Runtime::~Runtime() {
  while (runtime_active_ && !aborted_) {
    runScheduler();
  }
  if (!aborted_) {
    finalize();
  }
}

bool Runtime::tryInitialize() {
  bool const init_now = !initialized_ && !finalized_;

  debug_print(
    runtime, unknown,
    "Runtime: tryInitialize: initialized_={}, finalized_={}, init_now={}\n",
    print_bool(initialized_), print_bool(finalized_), print_bool(init_now)
  );

  if (init_now) {
    initialize(true);
  }
  return init_now;
}

bool Runtime::tryFinalize() {
  bool const rt_live = !finalized_ && initialized_;
  bool const has_run_sched = hasSchedRun();
  bool const finalize_now = rt_live && has_run_sched;

  debug_print(
    runtime, unknown,
    "Runtime: tryFinalize: initialized_={}, finalized_={}, rt_live={}, sched={}, "
    "finalize_now={}\n",
    print_bool(initialized_), print_bool(finalized_), print_bool(rt_live),
    print_bool(has_run_sched), print_bool(finalize_now)
  );

  if (finalize_now) {
    finalize(true);
  } else {
    finalize_on_term_ = true;
  }

  return finalize_now;
}

void Runtime::printStartupBanner() {
  NodeType const nodes = theContext->getNumNodes();
  WorkerCountType const workers = theContext->getNumWorkers();
  bool const has_workers = theContext->hasWorkers();

  std::string is_interop_str =
    is_interop_ ?
      std::string(" interop=") + std::string(is_interop_ ? "true:" : "false:") :
      std::string("");
  std::string init = "Runtime Initializing:" + is_interop_str;
  std::string mode = std::string("mode: ");
  std::string mode_type =
    std::string(num_workers_ == no_workers ? "single" : "multi") +
    std::string("-thread per rank");
  std::string thd = !has_workers ? std::string("") :
    std::string(", worker threading: ") +
    std::string(
      backend_enable_if_else(
        openmp, "OpenMP", backend_enable_if_else(stdthread, "std::thread", "")
      )
   );
  std::string cnt = !has_workers ? std::string("") :
    (std::string(", ") + std::to_string(workers) + std::string(" workers/node"));
  std::string node_str = nodes == 1 ? "node" : "nodes";
  std::string all_node = std::to_string(nodes) + " " + node_str + cnt;

  char hostname[1024];
  gethostname(hostname, 1024);

  auto green    = debug::green();
  auto red      = debug::red();
  auto reset    = debug::reset();
  auto bd_green = debug::bd_green();
  auto magenta  = debug::magenta();
  auto vt_pre   = debug::vtPre();
  auto emph     = [=](std::string s) -> std::string { return debug::emph(s); };
  auto reg      = [=](std::string s) -> std::string { return debug::reg(s);  };

  std::vector<std::string> features = {"" backend_print_all_features(0) };

  std::string dirty = "";
  if (strncmp(vt_git_clean_status.c_str(), "DIRTY", 5) == 0) {
    dirty = red + std::string("*dirty*") + reset;
  }

  auto f1 = fmt::format("{} {}{}\n", reg(init), reg(mode), emph(mode_type + thd));
  auto f2 = fmt::format("{}Running on: {}\n", green, emph(all_node));
  auto f3 = fmt::format("{}Machine Hostname: {}\n", green, emph(hostname));
  auto f4 = fmt::format("{}Build SHA: {}\n", green, emph(vt_git_sha1));
  auto f5 = fmt::format("{}Build Ref: {}\n", green, emph(vt_git_refspec));
  auto f6 = fmt::format("{}Description: {} {}\n", green, emph(vt_git_description), dirty);
  auto f7 = fmt::format("{}Compile-time Features Enabled:{}\n", green, reset);

  fmt::print("{}{}{}", vt_pre, f1, reset);
  fmt::print("{}{}{}", vt_pre, f2, reset);
  fmt::print("{}{}{}", vt_pre, f3, reset);
  fmt::print("{}{}{}", vt_pre, f4, reset);
  fmt::print("{}{}{}", vt_pre, f5, reset);
  fmt::print("{}{}{}", vt_pre, f6, reset);
  fmt::print("{}{}{}", vt_pre, f7, reset);
  for (auto i = 1; i < features.size(); i++) {
    fmt::print("{}\t{}\n", vt_pre, emph(features.at(i)));
  }

  auto warn_cr = [=](std::string opt, std::string compile) -> std::string {
    return fmt::format(
      "{}Warning:{} {}{}{} has no effect: compile-time"
      " feature {}{}{} is disabled{}\n", red, reset, magenta, opt, reset,
      magenta, compile, reset, reset
    );
  };
  auto opt_on = [=](std::string opt, std::string compile) -> std::string {
    return fmt::format(
      "{}Option:{} flag {}{}{} on: {}{}\n",
      green, reset, magenta, opt, reset, compile, reset
    );
  };
  auto opt_off = [=](std::string opt, std::string compile) -> std::string {
    return fmt::format(
      "{}Option:{} flag {}{}{} not set: {}{}\n",
      green, reset, magenta, opt, reset, compile, reset
    );
  };
  auto opt_inverse = [=](std::string opt, std::string compile) -> std::string {
    return fmt::format(
      "{}Default:{} {}, use {}{}{} to disable{}\n",
      green, reset, compile, magenta, opt, reset, reset
    );
  };

  auto f8 = fmt::format("{}Runtime Configuration:{}\n", green, reset);
  fmt::print("{}{}{}", vt_pre, f8, reset);

  #if !backend_check_enabled(lblite)
    if (ArgType::vt_lb) {
      auto f9 = warn_cr("--vt_lb", "lblite");
      fmt::print("{}\t{}{}", vt_pre, f9, reset);
    }
  #endif

  if (ArgType::vt_lb) {
    auto f9 = opt_on("--vt_lb", "Load balancing enabled");
    fmt::print("{}\t{}{}", vt_pre, f9, reset);
    if (ArgType::vt_lb_file) {
      auto f10 = opt_on("--vt_lb_file", "Reading LB config from file");
      fmt::print("{}\t{}{}", vt_pre, f10, reset);
      auto f12 = fmt::format("Reading file \"{}\"", ArgType::vt_lb_file_name);
      auto f11 = opt_on("--vt_lb_file_name", f12);
      fmt::print("{}\t{}{}", vt_pre, f11, reset);
    } else {
      auto a3 = fmt::format("Load balancer name: \"{}\"", ArgType::vt_lb_name);
      auto a4 = opt_on("--vt_lb_name", a3);
      fmt::print("{}\t{}{}", vt_pre, a4, reset);
      auto a1 =
        fmt::format("Load balancing interval = {}", ArgType::vt_lb_interval);
      auto a2 = opt_on("--vt_lb_interval", a1);
      fmt::print("{}\t{}{}", vt_pre, a2, reset);
    }
  }

  #if !backend_check_enabled(trace_enabled)
    if (ArgType::vt_trace) {
      auto f9 = warn_cr("--vt_trace", "trace_enabled");
      fmt::print("{}\t{}{}", vt_pre, f9, reset);
    }
  #endif

  if (ArgType::vt_trace) {
    auto f9 = opt_on("--vt_trace", "Tracing enabled");
    fmt::print("{}\t{}{}", vt_pre, f9, reset);
    if (ArgType::vt_trace_file != "") {
      auto f11 = fmt::format("Trace file name \"{}\"", theTrace->getTraceName());
      auto f12 = opt_on("--vt_trace_file", f11);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    } else {
      if (theTrace) {
        auto f11 = fmt::format("Trace file \"{}\"", theTrace->getTraceName());
        auto f12 = opt_inverse("--vt_trace_file", f11);
        fmt::print("{}\t{}{}", vt_pre, f12, reset);
      }
    }
    if (ArgType::vt_trace_dir != "") {
      auto f11 = fmt::format("Directory \"{}\"", ArgType::vt_trace_dir);
      auto f12 = opt_on("--vt_trace_dir", f11);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    } else {
      if (theTrace) {
        auto f11 = fmt::format(
          "Trace directory \"{}\"", theTrace->getDirectory()
        );
        auto f12 = opt_inverse("--vt_trace_dir", f11);
        fmt::print("{}\t{}{}", vt_pre, f12, reset);
      }
    }
    if (ArgType::vt_trace_mod != 0) {
      auto f11 = fmt::format("Output every {} files ", ArgType::vt_trace_mod);
      auto f12 = opt_on("--vt_trace_mod", f11);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    }
  }

  if (ArgType::vt_no_sigint) {
    auto f11 = fmt::format("Disabling SIGINT signal handling");
    auto f12 = opt_on("--vt_no_SIGINT", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  } else {
    auto f11 = fmt::format("SIGINT signal handling enabled by default");
    auto f12 = opt_inverse("--vt_no_SIGINT", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (ArgType::vt_no_sigsegv) {
    auto f11 = fmt::format("Disabling SIGSEGV signal handling");
    auto f12 = opt_on("--vt_no_SIGSEGV", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  } else {
    auto f11 = fmt::format("SIGSEGV signal handling enabled by default");
    auto f12 = opt_inverse("--vt_no_SIGSEGV", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (ArgType::vt_no_color) {
    auto f11 = fmt::format("Disabling color output");
    auto f12 = opt_on("--vt_no_color", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  } else {
    if (ArgType::vt_auto_color) {
      auto f11 = fmt::format("Automatic TTY detection for color output");
      auto f12 = opt_on("--vt_auto_color", f11);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    } else {
      auto f11 = fmt::format("Color output enabled by default");
      auto f12 = opt_inverse("--vt_no_color", f11);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    }
  }

  if (ArgType::vt_no_stack) {
    auto f11 = fmt::format("Disabling all stack dumps");
    auto f12 = opt_on("--vt_no_stack", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  } else {
    auto f11 = fmt::format("Stack dumps enabled by default");
    auto f12 = opt_inverse("--vt_no_stack", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (ArgType::vt_no_warn_stack) {
    auto f11 = fmt::format("Disabling all stack dumps on vtWarn(..)");
    auto f12 = opt_on("--vt_no_warn_stack", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (ArgType::vt_no_assert_stack) {
    auto f11 = fmt::format("Disabling all stack dumps on vtAssert(..)");
    auto f12 = opt_on("--vt_no_assert_stack", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (ArgType::vt_no_abort_stack) {
    auto f11 = fmt::format("Disabling all stack dumps on vtAbort(..)");
    auto f12 = opt_on("--vt_no_abort_stack", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (ArgType::vt_stack_file != "") {
    auto f11 = fmt::format(
      "Output stack dumps with file name {}", ArgType::vt_stack_file
    );
    auto f12 = opt_on("--vt_stack_file", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (ArgType::vt_stack_dir != "") {
    auto f11 = fmt::format("Output stack dumps to {}", ArgType::vt_stack_dir);
    auto f12 = opt_on("--vt_stack_dir", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (ArgType::vt_stack_mod != 0) {
    auto f11 = fmt::format(
      "Output stack dumps every {} files ", ArgType::vt_stack_mod
    );
    auto f12 = opt_on("--vt_stack_mod", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (ArgType::vt_debug_all) {
    auto f11 = fmt::format("All debug prints are on (if enabled compile-time)");
    auto f12 = opt_on("--vt_debug_all", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

#define debug_warn_compile(opt)                               \
  do {                                                        \
    if (ArgType::vt_debug_ ## opt) {                          \
      auto f9 = warn_cr("--vt_debug_" #opt, "debug_" #opt);   \
      fmt::print("{}\t{}{}", vt_pre, f9, reset);              \
    }                                                         \
  } while (0);

  #if !backend_check_enabled(none)
    debug_warn_compile(none)
  #endif
  #if !backend_check_enabled(gen)
    debug_warn_compile(gen)
  #endif
  #if !backend_check_enabled(runtime)
    debug_warn_compile(runtime)
  #endif
  #if !backend_check_enabled(active)
    debug_warn_compile(active)
  #endif
  #if !backend_check_enabled(term)
    debug_warn_compile(term)
  #endif
  #if !backend_check_enabled(termds)
    debug_warn_compile(termds)
  #endif
  #if !backend_check_enabled(barrier)
    debug_warn_compile(barrier)
  #endif
  #if !backend_check_enabled(event)
    debug_warn_compile(event)
  #endif
  #if !backend_check_enabled(pipe)
    debug_warn_compile(pipe)
  #endif
  #if !backend_check_enabled(pool)
    debug_warn_compile(pool)
  #endif
  #if !backend_check_enabled(reduce)
    debug_warn_compile(reduce)
  #endif
  #if !backend_check_enabled(rdma)
    debug_warn_compile(rdma)
  #endif
  #if !backend_check_enabled(rdma_channel)
    debug_warn_compile(rdma_channel)
  #endif
  #if !backend_check_enabled(rdma_state)
    debug_warn_compile(rdma_state)
  #endif
  #if !backend_check_enabled(param)
    debug_warn_compile(param)
  #endif
  #if !backend_check_enabled(handler)
    debug_warn_compile(handler)
  #endif
  #if !backend_check_enabled(hierlb)
    debug_warn_compile(hierlb)
  #endif
  #if !backend_check_enabled(scatter)
      debug_warn_compile(scatter)
  #endif
  #if !backend_check_enabled(sequence)
    debug_warn_compile(sequence)
  #endif
  #if !backend_check_enabled(sequence_vrt)
    debug_warn_compile(sequence_vrt)
  #endif
  #if !backend_check_enabled(serial_msg)
    debug_warn_compile(serial_msg)
  #endif
  #if !backend_check_enabled(trace)
    debug_warn_compile(trace)
  #endif
  #if !backend_check_enabled(location)
    debug_warn_compile(location)
  #endif
  #if !backend_check_enabled(lb)
    debug_warn_compile(lb)
  #endif
  #if !backend_check_enabled(vrt)
    debug_warn_compile(vrt)
  #endif
  #if !backend_check_enabled(vrt_coll)
    debug_warn_compile(vrt_coll)
  #endif
  #if !backend_check_enabled(worker)
    debug_warn_compile(worker)
  #endif
  #if !backend_check_enabled(group)
    debug_warn_compile(group)
  #endif
  #if !backend_check_enabled(broadcast)
    debug_warn_compile(broadcast)
  #endif

  //fmt::print("{}\n", reset);
  fmt::print(reset);
}

void Runtime::printShutdownBanner(term::TermCounterType const& num_units) {
  auto green    = debug::green();
  auto reset    = debug::reset();
  auto bd_green = debug::bd_green();
  auto magenta  = debug::magenta();
  auto f1 = fmt::format("{}Runtime Finalizing{}", green, reset);
  auto f2 = fmt::format("{}Total work units processed:{} ", green, reset);
  auto vt_pre = bd_green + std::string("vt") + reset + ": ";
  std::string fin = "";
  std::string units = std::to_string(num_units);
  fmt::print("{}{}{}{}{}\n", vt_pre, f2, magenta, units, reset);
  fmt::print("{}{}{}\n", vt_pre, f1, reset);
}

bool Runtime::initialize(bool const force_now) {
  if (force_now) {
    initializeContext(user_argc_, user_argv_, communicator_);
    initializeComponents();
    initializeOptionalComponents();
    sync();
    if (theContext->getNode() == 0) {
      printStartupBanner();
    }
    setup();
    sync();
    initialized_ = true;
    return true;
  } else {
    return tryInitialize();
  }
}

bool Runtime::finalize(bool const force_now) {
  if (force_now) {
    auto const& is_zero = theContext->getNode() == 0;
    auto const& num_units = theTerm->getNumUnits();
    sync();
    fflush(stdout);
    fflush(stderr);
    sync();
    finalizeComponents();
    finalizeOptionalComponents();
    finalizeTrace();
    sync();
    sync();
    if (is_zero) {
      printShutdownBanner(num_units);
    }
    finalizeContext();
    finalized_ = true;
    return true;
  } else {
    return tryFinalize();
  }
}

void Runtime::sync() {
  MPI_Barrier(theContext->getComm());
}

void Runtime::runScheduler() {
  theSched->scheduler();
}

void Runtime::reset() {
  MPI_Barrier(theContext->getComm());

  runtime_active_ = true;
  theTerm->resetGlobalTerm();

  MPI_Barrier(theContext->getComm());
}

void Runtime::abort(std::string const abort_str, ErrorCodeType const code) {
  aborted_ = true;
  output(abort_str,code,true,true,false);
  _Exit(code);
  // @todo: why will this not compile with clang!!?
  //quick_exit(code);
}

void Runtime::output(
  std::string const abort_str, ErrorCodeType const code, bool error,
  bool decorate, bool formatted
) {
  auto node      = theContext ? theContext->getNode() : -1;
  auto green     = debug::green();
  auto byellow   = debug::byellow();
  auto red       = debug::red();
  auto reset     = debug::reset();
  auto vt_pre    = debug::vtPre();
  auto bred      = debug::bred();
  auto node_str  = ::vt::debug::proc(node);
  auto prefix    = vt_pre + node_str + " ";
  auto seperator = fmt::format("{}{}{:-^120}{}\n", prefix, bred, "", reset);
  auto warn_sep  = fmt::format("{}{}{:-^120}{}\n", prefix, byellow, "", reset);
  // auto space     = fmt::format("{}\n", prefix);

  if (decorate) {
    if (error) {
      auto f1 = fmt::format(" Runtime Error: System Aborting! ");
      auto const info = ::fmt::format(" Fatal Error on Node {} ", node);
      // fmt::print(stderr, "{}", space);
      fmt::print(stderr, "{}", seperator);
      fmt::print(stderr, "{}{}{:-^120}{}\n", prefix, bred, f1, reset);
      fmt::print(stderr, "{}{}{:-^120}{}\n", prefix, bred, info, reset);
      fmt::print(stderr, "{}", seperator);
    } else {
      auto f1 = fmt::format(" Runtime Warning ");
      auto const info = ::fmt::format(" Warning on Node {} ", node);
      // fmt::print(stderr, "{}", space);
      fmt::print(stderr, "{}", warn_sep);
      fmt::print(stderr, "{}{}{:-^120}{}\n", prefix, byellow, f1,   reset);
      fmt::print(stderr, "{}{}{:-^120}{}\n", prefix, byellow, info, reset);
      fmt::print(stderr, "{}", warn_sep);
    }
  }

  if (formatted) {
    fmt::print(stderr, "{}", abort_str);
  } else {
    fmt::print(stderr, "{}\n", prefix);
    fmt::print(stderr, "{}Message: {}{}{}\n", prefix, byellow, abort_str, reset);
    fmt::print(stderr, "{}\n", prefix);
  }

  if (!ArgType::vt_no_stack) {
    bool const on_abort = !ArgType::vt_no_abort_stack;
    bool const on_warn = !ArgType::vt_no_warn_stack;
    bool const dump = (error && on_abort) || (!error && on_warn);
    if (dump) {
      if (Runtime::nodeStackWrite()) {
        auto stack = debug::stack::dumpStack();
        auto stack_pretty = debug::stack::prettyPrintStack(std::get<1>(stack));
        if (ArgType::vt_stack_file != "") {
          Runtime::writeToFile(stack_pretty);
        } else {
          fmt::print("{}", stack_pretty);
        }
      }
    }
  }

  fflush(stdout);
  fflush(stderr);
}

void Runtime::terminationHandler() {
  debug_print(
    runtime, node,
    "Runtime: executing registered termination handler\n",
  );

  runtime_active_ = false;

  if (finalize_on_term_) {
    finalize();
  }
}

void Runtime::setup() {
  debug_print(runtime, node, "begin: setup\n");

  runtime_active_ = true;

  auto action = std::bind(&Runtime::terminationHandler, this);
  theTerm->addDefaultAction(action);

  MPI_Barrier(theContext->getComm());

  // wait for all nodes to start up to initialize the runtime
  theCollective->barrierThen([this]{
    MPI_Barrier(theContext->getComm());
  });

  debug_print(runtime, node, "end: setup\n");
}

void Runtime::initializeContext(int argc, char** argv, MPI_Comm* comm) {
  theContext = std::make_unique<ctx::Context>(argc, argv, is_interop_, comm);

  debug_print(runtime, node, "finished initializing context\n");
}

void Runtime::finalizeContext() {
  MPI_Barrier(theContext->getComm());

  if (not is_interop_) {
    MPI_Finalize();
  }
}

void Runtime::initializeComponents() {
  debug_print(runtime, node, "begin: initializeComponents\n");

  // Helper components: not allowed to send messages during construction
  theRegistry = std::make_unique<registry::Registry>();
  theEvent = std::make_unique<event::AsyncEvent>();
  thePool = std::make_unique<pool::Pool>();

  // Core components: enables more complex subsequent initialization
  theMsg = std::make_unique<messaging::ActiveMessenger>();
  theSched = std::make_unique<sched::Scheduler>();
  initializeTrace();
  theTerm = std::make_unique<term::TerminationDetector>();
  theCollective = std::make_unique<collective::CollectiveAlg>();
  theGroup = std::make_unique<group::GroupManager>();
  theCB = std::make_unique<pipe::PipeManager>();

  // Advanced runtime components: not required for basic messaging
  theRDMA = std::make_unique<rdma::RDMAManager>();
  theParam = std::make_unique<param::Param>();
  theSeq = std::make_unique<seq::Sequencer>();
  theVirtualSeq = std::make_unique<seq::SequencerVirtual>();
  theLocMan = std::make_unique<location::LocationManager>();
  theVirtualManager = std::make_unique<vrt::VirtualContextManager>();
  theCollection = std::make_unique<vrt::collection::CollectionManager>();

  debug_print(runtime, node, "end: initializeComponents\n");
}

void Runtime::initializeTrace() {
  backend_enable_if(
    trace_enabled, {
      theTrace = std::make_unique<trace::Trace>();
    }
  );

  backend_enable_if(
    trace_enabled, {
      std::string name = user_argc_ == 0 ? "prog" : user_argv_[0];
      auto const& node = theContext->getNode();
      theTrace->setupNames(
        name, name + "." + std::to_string(node) + ".log.gz", name + "_trace"
      );
    }
  );
}

void Runtime::finalizeTrace() {
  backend_enable_if(
    trace_enabled, {
      theTrace = nullptr;
    }
  );
}

void Runtime::initializeOptionalComponents() {
  debug_print(runtime, node, "begin: initializeOptionalComponents\n");

  initializeWorkers(num_workers_);

  debug_print(runtime, node, "end: initializeOptionalComponents\n");
}

void Runtime::initializeWorkers(WorkerCountType const num_workers) {
  using ::vt::ctx::ContextAttorney;

  debug_print(
    runtime, node, "begin: initializeWorkers: workers={}\n", num_workers
  );

  bool const has_workers = num_workers != no_workers;

  if (has_workers) {
    ContextAttorney::setNumWorkers(num_workers);

    // Initialize individual memory pool for each worker
    thePool->initWorkerPools(num_workers);

    theWorkerGrp = std::make_unique<worker::WorkerGroupType>();

    auto localTermFn = [](worker::eWorkerGroupEvent event){
      bool const no_local_workers = false;
      bool const is_idle = event == worker::eWorkerGroupEvent::WorkersIdle;
      bool const is_busy = event == worker::eWorkerGroupEvent::WorkersBusy;
      if (is_idle || is_busy) {
        ::vt::theTerm()->setLocalTerminated(is_idle, no_local_workers);
      }
    };
    theWorkerGrp->registerIdleListener(localTermFn);
  } else {
    // Without workers running on the node, the termination detector should
    // assume its locally ready to propagate instead of waiting for them to
    // become idle.
    theTerm->setLocalTerminated(true);
  }

  debug_print(runtime, node, "end: initializeWorkers\n");
}

void Runtime::finalizeComponents() {
  debug_print(runtime, node, "begin: finalizeComponents\n");

  // Reverse order destruction of runtime components.

  // Advanced components: may communicate during destruction
  theCollection = nullptr;
  theVirtualManager = nullptr;
  theLocMan = nullptr;
  theVirtualSeq = nullptr;
  theSeq = nullptr;
  theParam = nullptr;
  theRDMA = nullptr;

  // Core components
  theCollective = nullptr;
  theTerm = nullptr;
  theSched = nullptr;
  theMsg = nullptr;
  theGroup = nullptr;
  theCB = nullptr;

  // Helper components: thePool the last to be destructed because it handles
  // memory allocations
  theRegistry = nullptr;
  theEvent->cleanup(); theEvent = nullptr;

  // Initialize individual memory pool for each worker
  thePool->destroyWorkerPools();
  thePool = nullptr;

  debug_print(runtime, node, "end: finalizeComponents\n");
}

void Runtime::finalizeOptionalComponents() {
  debug_print(runtime, node, "begin: finalizeOptionalComponents\n");

  theWorkerGrp = nullptr;

  debug_print(runtime, node, "end: finalizeOptionalComponents\n");
}

}} //end namespace vt::runtime
