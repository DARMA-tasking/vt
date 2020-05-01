/*
//@HEADER
// *****************************************************************************
//
//                                  runtime.cc
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

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
#include "vt/pipe/pipe_manager.h"
#include "vt/objgroup/manager.h"
#include "vt/scheduler/scheduler.h"
#include "vt/termination/termination.h"
#include "vt/topos/location/location_headers.h"
#include "vt/vrt/context/context_vrtmanager.h"
#include "vt/vrt/collection/collection_headers.h"
#include "vt/worker/worker_headers.h"
#include "vt/configs/generated/vt_git_revision.h"
#include "vt/configs/debug/debug_colorize.h"
#include "vt/configs/arguments/args.h"
#include "vt/configs/error/stack_out.h"
#include "vt/configs/error/pretty_print_stack.h"
#include "vt/utils/memory/memory_usage.h"
#include "vt/utils/mpi_limits/mpi_max_tag.h"

#include <memory>
#include <iostream>
#include <functional>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <limits.h>
#include <unistd.h>
#include <csignal>

namespace vt { namespace runtime {

/*static*/ std::string Runtime::prog_name_ = "";
/*static*/ bool volatile Runtime::sig_user_1_ = false;

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
  sig_user_1_ = false;
  setupSignalHandler();
  setupSignalHandlerINT();
  setupTerminateHandler();
}

bool Runtime::hasSchedRun() const {
  return theSched ? theSched->hasSchedRun() : false;
}

void Runtime::pauseForDebugger() {
  if (ArgType::vt_pause) {
    char node_str[256];
    auto node = vt::theContext() ? vt::theContext()->getNode() : -1;
    sprintf(node_str, "prog-%d.pid", node);
    auto const pid = getpid();
    FILE* f = fopen(node_str, "w+");
    fprintf(f, "%d", pid);
    fclose(f);
    while (!sig_user_1_);
  }
}

/*static*/ void Runtime::sigHandlerINT(int sig) {
  auto node      = vt::theContext() ? vt::theContext()->getNode() : -1;
  auto vt_pre    = debug::vtPre();
  auto node_str  = ::vt::debug::proc(node);
  auto prefix    = vt_pre + node_str + " ";
  if (node == 0 || node == -1) {
    ::fmt::print("{}Caught SIGINT signal: {} \n", prefix, sig);
  }
  // Try to flush out all logs before dying
# if backend_check_enabled(trace_enabled)
  if (vt::theTrace()) {
    vt::theTrace()->cleanupTracesFile();
  }
# endif
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
  std::_Exit(EXIT_FAILURE);
}

/*static*/ void Runtime::sigHandlerUsr1(int sig) {
  Runtime::sig_user_1_ = true;
}

/*static*/ void Runtime::sigHandler(int sig) {
  auto vt_pre    = debug::vtPre();
  auto bred      = debug::bred();
  ::fmt::print("{}Caught SIGSEGV signal: {} \n", vt_pre, sig);
  // Try to flush out all logs before dying
# if backend_check_enabled(trace_enabled)
  if (vt::theTrace()) {
    vt::theTrace()->cleanupTracesFile();
  }
# endif
  if (Runtime::nodeStackWrite()) {
    auto stack = debug::stack::dumpStack();
    if (ArgType::vt_stack_file != "") {
      Runtime::writeToFile(std::get<0>(stack));
    } else {
      ::fmt::print("{}{}{}\n", bred, std::get<0>(stack), debug::reset());
      ::fmt::print("\n");
    }
  }
  std::_Exit(EXIT_FAILURE);
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
  std::_Exit(EXIT_FAILURE);
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
  signal(SIGUSR1, Runtime::sigHandlerUsr1);
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
  theSched->runSchedulerWhile([this]{
    return runtime_active_ && !aborted_;
  });
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
  // If --vt_quiet is set, immediately exit printing nothing during startup
  if (ArgType::vt_quiet) {
    return;
  }

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
      #if backend_check_enabled(openmp)
        "OpenMP"
      #elif backend_check_enabled(stdthread)
        "std::thread"
      #else
        ""
      #endif
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

  std::vector<std::string> features;

#if backend_check_enabled(bit_check_overflow)
  features.push_back(vt_feature_str_bit_check_overflow);
#endif
#if backend_check_enabled(trace_enabled)
  features.push_back(vt_feature_str_trace_enabled);
#endif
#if backend_check_enabled(detector)
  features.push_back(vt_feature_str_detector);
#endif
#if backend_check_enabled(lblite)
  features.push_back(vt_feature_str_lblite);
#endif
#if backend_check_enabled(openmp)
  features.push_back(vt_feature_str_openmp);
#endif
#if backend_check_enabled(production)
  features.push_back(vt_feature_str_production);
#endif
#if backend_check_enabled(priorities)
  features.push_back(vt_feature_str_priorities);
#endif
#if backend_check_enabled(stdthread)
  features.push_back(vt_feature_str_stdthread);
#endif
#if backend_check_enabled(mpi_rdma)
  features.push_back(vt_feature_str_mpi_rdma);
#endif
#if backend_check_enabled(print_term_msgs)
  features.push_back(vt_feature_str_print_term_msgs);
#endif
#if backend_check_enabled(no_pool_alloc_env)
  features.push_back(vt_feature_str_no_pool_alloc_env);
#endif
#if backend_check_enabled(memory_pool)
  features.push_back(vt_feature_str_memory_pool);
#endif

  std::string dirty = "";
  if (strncmp(vt_git_clean_status.c_str(), "DIRTY", 5) == 0) {
    dirty = red + std::string("*dirty*") + reset;
  }

  auto const max_tag = util::MPI_Attr::getMaxTag();
  auto const max_tag_str = std::to_string(max_tag);
  auto const version_tuple = util::MPI_Attr::getVersion();
  auto const version = std::to_string(std::get<0>(version_tuple));
  auto const subversion = std::to_string(std::get<1>(version_tuple));

  auto f1 = fmt::format("{} {}{}\n", reg(init), reg(mode), emph(mode_type + thd));
  auto f2 = fmt::format("{}Running on: {}\n", green, emph(all_node));
  auto f3 = fmt::format("{}Machine Hostname: {}\n", green, emph(hostname));
  auto f3a = fmt::format("{}MPI Version: {}.{}\n", green, emph(version), emph(subversion));
  auto f3b = fmt::format("{}MPI Max tag: {}\n", green, emph(max_tag_str));

  auto f4 = fmt::format("{}Build SHA: {}\n", green, emph(vt_git_sha1));
  auto f5 = fmt::format("{}Build Ref: {}\n", green, emph(vt_git_refspec));
  auto f6 = fmt::format("{}Description: {} {}\n", green, emph(vt_git_description), dirty);
  auto f7 = fmt::format("{}Compile-time Features Enabled:{}\n", green, reset);

  fmt::print("{}{}{}", vt_pre, f1, reset);
  fmt::print("{}{}{}", vt_pre, f2, reset);
  fmt::print("{}{}{}", vt_pre, f3, reset);
  fmt::print("{}{}{}", vt_pre, f3a, reset);
  fmt::print("{}{}{}", vt_pre, f3b, reset);
  fmt::print("{}{}{}", vt_pre, f4, reset);
  fmt::print("{}{}{}", vt_pre, f5, reset);
  fmt::print("{}{}{}", vt_pre, f6, reset);
  fmt::print("{}{}{}", vt_pre, f7, reset);
  for (size_t i = 0; i < features.size(); i++) {
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
  auto opt_to_enable = [=](std::string opt, std::string compile) -> std::string {
    return fmt::format(
      "{}Default:{} {}, use {}{}{} to enable{}\n",
      green, reset, compile, magenta, opt, reset, reset
    );
  };

  auto f8 = fmt::format("{}Runtime Configuration:{}\n", green, reset);
  fmt::print("{}{}{}", vt_pre, f8, reset);

  #if !backend_check_enabled(lblite)
    if (ArgType::vt_lb) {
      auto f9 = warn_cr("--vt_lb", "lblite");
      fmt::print("{}\t{}{}", vt_pre, f9, reset);
      vtAbort("Load balancing enabled with --vt_lb, but disabled at compile time");
    }
    if (ArgType::vt_lb_stats) {
      auto f9 = warn_cr("--vt_lb_stats", "lblite");
      fmt::print("{}\t{}{}", vt_pre, f9, reset);
    }
  #endif

  {
    auto f11 = fmt::format(
      "Running MPI progress {} times each invocation",
      ArgType::vt_sched_num_progress
    );
    auto f12 = opt_on("--vt_sched_num_progress", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  {
    auto f11 = fmt::format(
      "Running MPI progress function at least every {} handler(s) executed",
      ArgType::vt_sched_progress_han
    );
    auto f12 = opt_on("--vt_sched_progress_han", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (ArgType::vt_sched_progress_sec != 0.0) {
    auto f11 = fmt::format(
      "Running MPI progress function at least every {} seconds",
      ArgType::vt_sched_progress_sec
    );
    auto f12 = opt_on("--vt_sched_progress_sec", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (ArgType::vt_lb) {
    auto f9 = opt_on("--vt_lb", "Load balancing enabled");
    fmt::print("{}\t{}{}", vt_pre, f9, reset);
    if (ArgType::vt_lb_file) {
      if (ArgType::vt_lb_file_name == "") {
        auto warn_lb_file = fmt::format(
          "{}Warning:{} {}{}{} has no effect: compile-time"
          " option {}{}{} is empty{}\n", red, reset, magenta, "--vt_lb_file",
          reset, magenta, "--vt_lb_file_name", reset, reset
        );
        fmt::print("{}\t{}{}", vt_pre, warn_lb_file, reset);
      } else {
        auto f10 = opt_on("--vt_lb_file", "Reading LB config from file");
        fmt::print("{}\t{}{}", vt_pre, f10, reset);
        auto f12 = fmt::format("Reading file \"{}\"", ArgType::vt_lb_file_name);
        auto f11 = opt_on("--vt_lb_file_name", f12);
        fmt::print("{}\t{}{}", vt_pre, f11, reset);
      }
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

  if (ArgType::vt_lb_stats) {
    auto f9 = opt_on("--vt_lb_stats", "Load balancing statistics collection");
    fmt::print("{}\t{}{}", vt_pre, f9, reset);

    auto const fname = ArgType::vt_lb_stats_file;
    if (fname != "") {
      auto f11 = fmt::format("LB stats file name \"{}.0.out\"", fname);
      auto f12 = opt_on("--vt_lb_stats_file", f11);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    }

    auto const fdir = ArgType::vt_lb_stats_dir;
    if (fdir != "") {
      auto f11 = fmt::format("LB stats directory \"{}\"", fdir);
      auto f12 = opt_on("--vt_lb_stats_dir", f11);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    }
  }


  #if !backend_check_enabled(trace_enabled)
    if (ArgType::vt_trace) {
      auto f9 = warn_cr("--vt_trace", "trace_enabled");
      fmt::print("{}\t{}{}", vt_pre, f9, reset);
    }
  #endif

  #if backend_check_enabled(trace_enabled)
  if (ArgType::vt_trace) {
    auto f9 = opt_on("--vt_trace", "Tracing enabled");
    fmt::print("{}\t{}{}", vt_pre, f9, reset);
    if (ArgType::vt_trace_file != "") {
      auto f11 = fmt::format("Trace file name \"{}\"", ArgType::vt_trace_file);
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
    if (ArgType::vt_trace_flush_size != 0) {
      auto f11 = fmt::format("Flush output incrementally with a buffer of,"
                             " at least, {} record(s)",
                             ArgType::vt_trace_flush_size);
      auto f12 = opt_on("--vt_trace_flush_size", f11);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    } else {
      auto f11 = fmt::format("Flushing traces at end of run");
      auto f12 = opt_inverse("--vt_trace_flush_size", f11);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    }
    if (ArgType::vt_trace_spec) {
      {
        auto f11 = fmt::format("Using trace enable specification for phases");
        auto f12 = opt_on("--vt_trace_spec", f11);
        fmt::print("{}\t{}{}", vt_pre, f12, reset);
      }
      if (ArgType::vt_trace_spec_file == "") {
        auto warn_trace_file = fmt::format(
          "{}Warning:{} {}{}{} has no effect: no specification file given"
          " option {}{}{} is empty{}\n", red, reset, magenta,
          "--vt_trace_spec",
          reset, magenta, "--vt_trace_spec_file", reset, reset
        );
        fmt::print("{}\t{}{}", vt_pre, warn_trace_file, reset);
      } else {
        auto f11 = fmt::format(
          "Using trace specification file \"{}\"",
          ArgType::vt_trace_spec_file
        );
        auto f12 = opt_inverse("--vt_trace_spec", f11);
        fmt::print("{}\t{}{}", vt_pre, f12, reset);
      }
    }
    if (ArgType::vt_trace_memory_usage) {
      auto f11 = fmt::format("Tracing memory usage");
      auto f12 = opt_on("--vt_trace_memory_usage", f11);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    }
    if (ArgType::vt_trace_mpi) {
      auto f11 = fmt::format("Tracing MPI invocations");
      auto f12 = opt_on("--vt_trace_mpi", f11);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    }
    if (ArgType::vt_trace_event_polling) {
      auto f11 = fmt::format("Tracing event polling (inc. MPI Isend requests)");
      auto f12 = opt_on("--vt_trace_event_polling", f11);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    }
    if (ArgType::vt_trace_irecv_polling) {
      auto f11 = fmt::format("Tracing MPI Irecv polling");
      auto f12 = opt_on("--vt_trace_irecv_polling", f11);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    }
  }
  #endif


  if (ArgType::vt_term_rooted_use_ds) {
    auto f11 = fmt::format("Forcing the use of Dijkstra-Scholten for rooted TD");
    auto f12 = opt_on("--vt_term_rooted_use_ds", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (ArgType::vt_term_rooted_use_wave) {
    auto f11 = fmt::format("Forcing the use of 4-counter wave-based for rooted TD");
    auto f12 = opt_on("--vt_term_rooted_use_wave", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (ArgType::vt_print_no_progress) {
    auto f11 = fmt::format("Printing warnings when progress is stalls");
    auto f12 = opt_on("--vt_print_no_progress", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (ArgType::vt_epoch_graph_terse) {
    auto f11 = fmt::format("Printing terse epoch graphs when hang detected");
    auto f12 = opt_on("--vt_epoch_graph_terse", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  } else {
    auto f11 = fmt::format("Printing verbose epoch graphs when hang detected");
    auto f12 = opt_inverse("--vt_epoch_graph_terse", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (ArgType::vt_epoch_graph_on_hang) {
    auto f11 = fmt::format("Epoch graph output enabled if hang detected");
    auto f12 = opt_on("--vt_epoch_graph_on_hang", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (ArgType::vt_no_detect_hang) {
    auto f11 = fmt::format("Disabling termination hang detection");
    auto f12 = opt_on("--vt_no_detect_hang", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  } else {
    auto f11 = fmt::format("Termination hang detection enabled by default");
    auto f12 = opt_inverse("--vt_no_detect_hang", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (!ArgType::vt_no_detect_hang) {
    if (ArgType::vt_hang_freq != 0) {
      auto f11 = fmt::format(
        "Printing stall warning every {} tree traversals ", ArgType::vt_hang_freq
      );
      auto f12 = opt_on("--vt_hang_detect", f11);
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

  if (ArgType::vt_no_terminate) {
    auto f11 = fmt::format("Disabling std::terminate signal handling");
    auto f12 = opt_on("--vt_no_terminate", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  } else {
    auto f11 = fmt::format("std::terminate signal handling enabled by default");
    auto f12 = opt_inverse("--vt_no_terminate", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (ArgType::vt_no_color) {
    auto f11 = fmt::format("Color output disabled");
    auto f12 = opt_on("--vt_no_color", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  } else {
    auto f11 = fmt::format("Color output enabled");
    auto f12 = opt_inverse("--vt_no_color", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
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

  if (ArgType::vt_pause) {
    auto f11 = fmt::format("Enabled debug pause at startup");
    auto f12 = opt_on("--vt_pause", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (ArgType::vt_memory_reporters != "") {
    auto f11 = fmt::format(
      "Memory usage checker precedence: {}",
      ArgType::vt_memory_reporters
    );
    auto f12 = opt_on("--vt_memory_reporters", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);

    auto usage = util::memory::MemoryUsage::get();

    auto working_reporters = usage->getWorkingReporters();
    if (working_reporters.size() > 0) {
      std::string working_str = "";
      for (std::size_t i = 0; i < working_reporters.size(); i++) {
        working_str += working_reporters[i];
        if (i != working_reporters.size() - 1) {
          working_str += ",";
        }
      }
      auto f13 = fmt::format(
        "{}Working memory reporters:{} {}{}{}\n",
        green, reset, magenta, working_str, reset
      );
      fmt::print("{}\t{}{}", vt_pre, f13, reset);

      auto all_usage_str = usage->getUsageAll();
      if (all_usage_str != "") {
        auto f14 = fmt::format(
          "{}Initial memory usage:{} {}\n",
          green, reset, all_usage_str
        );
        fmt::print("{}\t{}{}", vt_pre, f14, reset);
      }
    }

    if (ArgType::vt_print_memory_each_phase) {
      auto f15 = fmt::format("Printing memory usage each phase");
      auto f16 = opt_on("--vt_print_memory_each_phase", f15);
      fmt::print("{}\t{}{}", vt_pre, f16, reset);

      auto f17 = fmt::format(
        "Printing memory usage from node: {}", ArgType::vt_print_memory_node
      );
      auto f18 = opt_on("--vt_print_memory_node", f17);
      fmt::print("{}\t{}{}", vt_pre, f18, reset);
    } else {
      auto f15 = fmt::format("Memory usage printing disabled");
      auto f16 = opt_to_enable("--vt_print_memory_each_phase", f15);
      fmt::print("{}\t{}{}", vt_pre, f16, reset);
    }

    if (ArgType::vt_print_memory_at_threshold) {
      auto f15 = fmt::format("Printing memory usage at threshold increment");
      auto f16 = opt_on("--vt_print_memory_at_threshold", f15);
      fmt::print("{}\t{}{}", vt_pre, f16, reset);

      auto f17 = fmt::format(
        "Printing memory usage using threshold: {}",
        ArgType::vt_print_memory_threshold
      );
      auto f18 = opt_on("--vt_print_memory_threshold", f17);
      fmt::print("{}\t{}{}", vt_pre, f18, reset);

      usage->convertBytesFromString(ArgType::vt_print_memory_threshold);

      auto f19 = fmt::format(
        "Polling for memory usage threshold every {} scheduler calls",
        ArgType::vt_print_memory_sched_poll
      );
      auto f20 = opt_on("--vt_print_memory_sched_poll", f19);
      fmt::print("{}\t{}{}", vt_pre, f20, reset);
    }
  }

  if (ArgType::vt_debug_all) {
    auto f11 = fmt::format("All debug prints are on (if enabled compile-time)");
    auto f12 = opt_on("--vt_debug_all", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (ArgType::vt_debug_print_flush) {
    auto f11 = fmt::format("Flushing stdout after all VT prints is enabled");
    auto f12 = opt_on("--vt_debug_print_flush", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

#define vt_runtime_debug_warn_compile(opt)                              \
  do {                                                                  \
    if (!vt_backend_debug_enabled(opt) and ArgType::vt_debug_ ## opt) { \
      auto f9 = warn_cr("--vt_debug_" #opt, "debug_" #opt);             \
      fmt::print("{}\t{}{}", vt_pre, f9, reset);                        \
    }                                                                   \
  } while (0);

  vt_runtime_debug_warn_compile(none)
  vt_runtime_debug_warn_compile(gen)
  vt_runtime_debug_warn_compile(runtime)
  vt_runtime_debug_warn_compile(active)
  vt_runtime_debug_warn_compile(term)
  vt_runtime_debug_warn_compile(termds)
  vt_runtime_debug_warn_compile(barrier)
  vt_runtime_debug_warn_compile(event)
  vt_runtime_debug_warn_compile(pipe)
  vt_runtime_debug_warn_compile(pool)
  vt_runtime_debug_warn_compile(reduce)
  vt_runtime_debug_warn_compile(rdma)
  vt_runtime_debug_warn_compile(rdma_channel)
  vt_runtime_debug_warn_compile(rdma_state)
  vt_runtime_debug_warn_compile(param)
  vt_runtime_debug_warn_compile(handler)
  vt_runtime_debug_warn_compile(hierlb)
  vt_runtime_debug_warn_compile(gossiplb)
  vt_runtime_debug_warn_compile(scatter)
  vt_runtime_debug_warn_compile(sequence)
  vt_runtime_debug_warn_compile(sequence_vrt)
  vt_runtime_debug_warn_compile(serial_msg)
  vt_runtime_debug_warn_compile(trace)
  vt_runtime_debug_warn_compile(location)
  vt_runtime_debug_warn_compile(vrt)
  vt_runtime_debug_warn_compile(vrt_coll)
  vt_runtime_debug_warn_compile(worker)
  vt_runtime_debug_warn_compile(group)
  vt_runtime_debug_warn_compile(broadcast)
  vt_runtime_debug_warn_compile(objgroup)

  //fmt::print("{}\n", reset);
  fmt::print(reset);

  // Enqueue a check for later in case arguments are modified before work
  // actually executes
  theSched->enqueue([this]{
    this->checkForArgumentErrors();
  });
}

void Runtime::printShutdownBanner(
  term::TermCounterType const& num_units, std::size_t const coll_epochs
) {
  // If --vt_quiet is set, immediately exit printing nothing during startup
  if (ArgType::vt_quiet) {
    return;
  }
  auto green    = debug::green();
  auto reset    = debug::reset();
  auto bd_green = debug::bd_green();
  auto magenta  = debug::magenta();
  auto f1 = fmt::format("{}Runtime Finalizing{}", green, reset);
  auto f2 = fmt::format("{}Total work units processed:{} ", green, reset);
  auto f3 = fmt::format("{}Total collective epochs processed:{} ", green, reset);
  auto vt_pre = bd_green + std::string("vt") + reset + ": ";
  std::string fin = "";
  std::string units = std::to_string(num_units);
  fmt::print("{}{}{}{}{}\n", vt_pre, f3, magenta, coll_epochs, reset);
  fmt::print("{}{}{}{}{}\n", vt_pre, f2, magenta, units, reset);
  fmt::print("{}{}{}\n", vt_pre, f1, reset);
}

void Runtime::checkForArgumentErrors() {
  #if !backend_check_enabled(lblite)
    if (ArgType::vt_lb) {
      vtAbort("Load balancing enabled with --vt_lb, but disabled at compile time");
    }
  #endif
}

bool Runtime::initialize(bool const force_now) {
  if (force_now) {
    initializeContext(user_argc_, user_argv_, communicator_);
    initializeComponents();
    initializeOptionalComponents();
    initializeErrorHandlers();
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
    auto const coll_epochs = theTerm->getNumTerminatedCollectiveEpochs();
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
      printShutdownBanner(num_units, coll_epochs);
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

  auto action = std::bind(&Runtime::terminationHandler, this);
  theTerm->addDefaultAction(action);
  theTerm->resetGlobalTerm();

  MPI_Barrier(theContext->getComm());

  // Without workers running on the node, the termination detector should
  // assume its locally ready to propagate instead of waiting for them to
  // become idle.
  theTerm->setLocalTerminated(true);
}

void Runtime::abort(std::string const abort_str, ErrorCodeType const code) {
  aborted_ = true;
  output(abort_str,code,true,true,false);
  std::raise( SIGTRAP );
  if (theContext) {
    auto const comm = theContext->getComm();
    MPI_Abort(comm, 129);
  } else {
    std::_Exit(code);
    // @todo: why will this not compile with clang!!?
    //quick_exit(code);
  }
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
    "Runtime: executing registered termination handler\n"
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

  // wait for all nodes to start up to initialize the runtime
  theCollective->barrier();

# if backend_check_enabled(trace_enabled)
  theTrace->loadAndBroadcastSpec();
# endif

  if (ArgType::vt_pause) {
    pauseForDebugger();
  }

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

  // Initialize the memory usage tracker on each node
  util::memory::MemoryUsage::initialize();

  // Helper components: not allowed to send messages during construction
  theRegistry = std::make_unique<registry::Registry>();
  theEvent = std::make_unique<event::AsyncEvent>();
  thePool = std::make_unique<pool::Pool>();

  // Initialize tracing, when it is enabled; used in the AM constructor
  initializeTrace();

  // Core components: enables more complex subsequent initialization
  theObjGroup = std::make_unique<objgroup::ObjGroupManager>();
  theMsg = std::make_unique<messaging::ActiveMessenger>();
  theSched = std::make_unique<sched::Scheduler>();
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

  #if backend_check_enabled(trace_enabled)
    theTrace->initialize();
  #endif
  theEvent->initialize();

  debug_print(runtime, node, "end: initializeComponents\n");
}

void Runtime::initializeTrace() {
  #if backend_check_enabled(trace_enabled)
    theTrace = std::make_unique<trace::Trace>();

    if (ArgType::vt_trace) {
      std::string name = user_argc_ == 0 ? "prog" : user_argv_[0];
      auto const& node = theContext->getNode();
      theTrace->setupNames(
        name, name + "." + std::to_string(node) + ".log.gz", name + "_trace"
      );
    }
  #endif
}

void Runtime::finalizeTrace() {
  #if backend_check_enabled(trace_enabled)
    theTrace = nullptr;
  #endif
}

namespace {
  /**
   * Error handler for MPI.
   * Called on any MPI error with the context's communicator. Aborts
   * the application. MPI calls are not guaranteed to work after
   * an error.
   *
   * \param comm    the MPI communicator
   * \param errc    pointer to the error code
   */
  void mpiErrorHandler(MPI_Comm *comm, int *errc, ...) {
    int len = MPI_MAX_ERROR_STRING;
    char msg[MPI_MAX_ERROR_STRING];
    MPI_Error_string(*errc, msg, &len);
    std::string err_msg(msg, len);

    fmt::print("{} (code: {})", err_msg, *errc);
    vtAbort("MPI Error");
  }
}

void Runtime::initializeErrorHandlers() {
  MPI_Errhandler err_handler = 0;
  MPI_Comm_create_errhandler(&mpiErrorHandler, &err_handler);
  MPI_Comm_set_errhandler(theContext->getComm(), err_handler);
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
    // enable/disable the global collective epoch based on the state of the
    // scheduler; register listeners to activate/deactivate that epoch
    theSched->registerTrigger(
      sched::SchedulerEvent::BeginIdleMinusTerm, []{
        debug_print(
          runtime, node,
          "setLocalTerminated: BeginIdle: true\n"
        );
        vt::theTerm()->setLocalTerminated(true, false);
      }
    );
    theSched->registerTrigger(
      sched::SchedulerEvent::EndIdleMinusTerm, []{
        debug_print(
          runtime, node,
          "setLocalTerminated: EndIdle: false\n"
        );
        vt::theTerm()->setLocalTerminated(false, false);
      }
    );
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
  theObjGroup = nullptr;

  // Helper components: thePool the last to be destructed because it handles
  // memory allocations
  theRegistry = nullptr;
  theEvent->cleanup(); theEvent = nullptr;

  // Initialize individual memory pool for each worker
  thePool->destroyWorkerPools();
  thePool = nullptr;

  // Finalize memory usage component
  util::memory::MemoryUsage::finalize();

  debug_print(runtime, node, "end: finalizeComponents\n");
}

void Runtime::finalizeOptionalComponents() {
  debug_print(runtime, node, "begin: finalizeOptionalComponents\n");

  theWorkerGrp = nullptr;

  debug_print(runtime, node, "end: finalizeOptionalComponents\n");
}

}} //end namespace vt::runtime
