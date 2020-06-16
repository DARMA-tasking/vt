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
#include "vt/pipe/pipe_manager.h"
#include "vt/objgroup/manager.h"
#include "vt/scheduler/scheduler.h"
#include "vt/topos/location/location_headers.h"
#include "vt/rdmahandle/manager.h"
#include "vt/vrt/context/context_vrtmanager.h"
#include "vt/vrt/collection/collection_headers.h"
#include "vt/vrt/collection/balance/lb_type.h"
#include "vt/worker/worker_headers.h"
#include "vt/configs/debug/debug_colorize.h"
#include "vt/configs/error/stack_out.h"
#include "vt/configs/error/pretty_print_stack.h"
#include "vt/utils/memory/memory_usage.h"
#include "vt/runtime/component/component_pack.h"
#include "vt/utils/mpi_limits/mpi_max_tag.h"
#include "vt/vrt/collection/balance/stats_restart_reader.h"

#include "vt/configs/arguments/argparse.h"
#include "vt/configs/arguments/args.h"

#include <memory>
#include <iostream>
#include <functional>
#include <string>
#include <vector>
#include <tuple>
#include <fstream>

#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <limits.h>
#include <unistd.h>
#include <csignal>

namespace vt { namespace runtime {

/*static*/ bool volatile Runtime::sig_user_1_ = false;

Runtime::Runtime(
  int& argc, char**& argv, WorkerCountType in_num_workers,
  bool const interop_mode, MPI_Comm* in_comm, RuntimeInstType const in_instance
)  : instance_(in_instance), runtime_active_(false), is_interop_(interop_mode),
     num_workers_(in_num_workers),
     communicator_(
       in_comm == nullptr ? MPI_COMM_NULL : *in_comm
     )
{
  // MPI_Init 'should' be called first on the original arguments,
  // with the justification that in some environments in addition to removing
  // special MPI arguments, it can actually ADD arguments not from argv.
  // That is not done here and doing so moves up parts of 'initialize' logic.

  // n.b. ref-update of args with pass-through arguments
  // (pass-through arguments are neither for VT or MPI_Init)
  std::tuple<int, std::string> result =
    arguments::ArgParse::parse(/*out*/ argc, /*out*/ argv);
  int exit_code = std::get<0>(result);

  if (exit_code not_eq -1) {
    // Help requested or invalid argument(s).
    // To better honor the MPI contract, force an MPI_Init then MPI_Abort.
    // It might be better to move up the general MPI_Init case; normally
    // MPI_Init is called as a result of Runtime::initialize (while this is ctor).
    MPI_Comm comm = communicator_ != MPI_COMM_NULL ? communicator_ : MPI_COMM_WORLD;
    int rank;
    MPI_Init(NULL, NULL);
    MPI_Comm_rank(comm, &rank);

    if (rank == 0) {
      // Only emit output to rank 0 to minimize spam
      std::string& msg = std::get<1>(result);
      // exit code of 0 -> 'help'
      std::ostream& out = exit_code == 0 ? std::cout : std::cerr;

      out << "--- VT INITIALIZATION ABORT ---" << "\n\n"
          << msg << "\n"
          << "--- VT INITIALIZATION ABORT ---" << "\n"
          << std::flush;
    }

    MPI_Abort(comm, exit_code);
    std::_Exit(exit_code); // no return
  }

  sig_user_1_ = false;
  setupSignalHandler();
  setupSignalHandlerINT();
  setupTerminateHandler();

  setupArgs();
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
    if (ArgType::vt_stack_file.empty()) {
      ::fmt::print("{}{}{}\n", bred, std::get<0>(stack), debug::reset());
      ::fmt::print("\n");
    } else {
      Runtime::writeToFile(std::get<0>(stack));
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
  std::string& app_name = ArgType::prog_name;
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
  if (runtime_active_ && !aborted_) {
    theSched->runSchedulerWhile([this]{
      return runtime_active_ && !aborted_;
    });
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

bool Runtime::needStatsRestartReader() {
  #if backend_check_enabled(lblite)
    if (ArgType::vt_lb_stats) {
      auto lbNames = vrt::collection::balance::lb_names_;
      auto mapLB = vrt::collection::balance::LBType::StatsMapLB;
      if (ArgType::vt_lb_name == lbNames[mapLB]) {
        return true;
      }
    }
  #endif
  return false;
}

bool Runtime::initialize(bool const force_now) {
  if (force_now) {
    initializeComponents();
    initializeOptionalComponents();
    initializeErrorHandlers();
    sync();
    if (theContext->getNode() == 0) {
      printStartupBanner();
      // Enqueue a check for later in case arguments are modified before work
      // actually executes
      theSched->enqueue([this]{
        this->checkForArgumentErrors();
      });

      // If the user specified to output a configuration file, write it to the
      // specified file on rank 0
      if (ArgType::vt_output_config) {
        std::ofstream out(ArgType::vt_output_config_file);
        out << ArgType::vt_output_config_str;
        out.close();
      }
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
    // This destroys and finalizes all components in proper reverse
    // initialization order
    p_.reset(nullptr);
    sync();
    sync();
    if (is_zero) {
      printShutdownBanner(num_units, coll_epochs);
    }
    finalizeMPI();
    finalized_ = true;
    return true;
  } else {
    return tryFinalize();
  }
}

void Runtime::sync() {
  MPI_Barrier(communicator_);
}

void Runtime::runScheduler() {
  theSched->scheduler();
}

void Runtime::reset() {
  sync();

  runtime_active_ = true;

  auto action = std::bind(&Runtime::terminationHandler, this);
  theTerm->addDefaultAction(action);
  theTerm->resetGlobalTerm();

  sync();

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

void Runtime::setupArgs() {
  std::vector<char*>& mpi_args = ArgType::mpi_init_args;
  user_argc_ = mpi_args.size() + 1;
  user_argv_ = std::make_unique<char*[]>(user_argc_ + 1);

  int i = 0;
  user_argv_[i++] = ArgType::argv_prog_name;
  for (char*& arg : mpi_args) {
    user_argv_[i++] = arg;
  }
  user_argv_[i++] = nullptr;
}

void Runtime::finalizeMPI() {
  sync();

  if (not is_interop_) {
    MPI_Finalize();
  }
}

void Runtime::initializeComponents() {
  debug_print(runtime, node, "begin: initializeComponents\n");

  using component::ComponentPack;
  using component::Deps;

  p_ = std::make_unique<ComponentPack>();

  p_->registerComponent<ctx::Context>(
    &theContext, Deps<>{},
    user_argc_, &user_argv_[0], is_interop_, &communicator_
  );

  p_->registerComponent<util::memory::MemoryUsage>(&theMemUsage, Deps<
    ctx::Context // Everything depends on theContext
  >{});

  p_->registerComponent<registry::Registry>(&theRegistry, Deps<
    ctx::Context // Everything depends on theContext
  >{});

  p_->registerComponent<pool::Pool>(&thePool, Deps<
    ctx::Context // Everything depends on theContext
  >{});

  p_->registerComponent<event::AsyncEvent>(&theEvent, Deps<
#   if backend_check_enabled(trace_enabled)
    trace::Trace,  // For trace user event registrations
#   endif
    ctx::Context,  // Everything depends on theContext
    pool::Pool     // For memory allocations
  >{});

# if backend_check_enabled(trace_enabled)
  // The Trace and Scheduler components have a co-dependency. However,
  // the lifetime of theTrace should be longer than that of theSched.
  p_->registerComponent<trace::Trace>(&theTrace, Deps<
      ctx::Context  // Everything depends on theContext
    >{},
    ArgType::prog_name
  );
# endif

  p_->registerComponent<objgroup::ObjGroupManager>(
    &theObjGroup, Deps<
      ctx::Context,              // Everything depends on theContext
      messaging::ActiveMessenger // Depends on active messenger to send
    >{}
  );

  p_->registerComponent<messaging::ActiveMessenger>(
    &theMsg, Deps<
#     if backend_check_enabled(trace_enabled)
      trace::Trace,      // For trace user event registrations
#     endif
      ctx::Context,      // Everything depends on theContext
      event::AsyncEvent, // Depends on event to send messages
      pool::Pool,        // Depends on pool for message allocation
      registry::Registry // Depends on registry for handlers
    >{}
  );

  p_->registerComponent<sched::Scheduler>(
    &theSched, Deps<
#     if backend_check_enabled(trace_enabled)
      trace::Trace,             // For scheduler-related trace events
#     endif
      ctx::Context,             // Everything depends on theContext
      util::memory::MemoryUsage // Depends on memory usage for output
    >{}
  );

  p_->registerComponent<term::TerminationDetector>(
    &theTerm, Deps<
      ctx::Context,               // Everything depends on theContext
      messaging::ActiveMessenger, // Depends on active messenger to send term msgs
      sched::Scheduler            // Depends on scheduler for idle checks
    >{}
  );

  p_->registerComponent<worker::WorkerGroupType>(
    &theWorkerGrp, Deps<
      ctx::Context,               // Everything depends on theContext
      messaging::ActiveMessenger, // Depends on active messenger to send msgs
      sched::Scheduler,           // Depends on scheduler
      term::TerminationDetector   // Depends on TD for idle callbacks
    >{}
  );

  p_->registerComponent<collective::CollectiveAlg>(
    &theCollective, Deps<
      ctx::Context,              // Everything depends on theContext
      messaging::ActiveMessenger // Depends on active messenger for collectives
    >{}
  );

  p_->registerComponent<group::GroupManager>(
    &theGroup, Deps<
      ctx::Context,               // Everything depends on theContext
      messaging::ActiveMessenger, // Depends on active messenger for setting up
      collective::CollectiveAlg   // Depends on collective for spanning trees
    >{}
  );

  p_->registerComponent<pipe::PipeManager>(
    &theCB, Deps<
      ctx::Context,                        // Everything depends on theContext
      messaging::ActiveMessenger,          // Depends on AM for callbacks
      collective::CollectiveAlg,           // Depends on collective for callbacks
      objgroup::ObjGroupManager,           // Depends on objgroup for callbacks
      vrt::collection::CollectionManager   // Depends collection for callbacks
    >{}
  );

  p_->registerComponent<rdma::RDMAManager>(
    &theRDMA, Deps<
      ctx::Context,                 // Everything depends on theContext
      messaging::ActiveMessenger,   // Depends on active messenger for RDMA
      collective::CollectiveAlg     // Depends on collective scope
    >{}
  );

  p_->registerComponent<param::Param>(
    &theParam, Deps<
      ctx::Context,              // Everything depends on theContext
      messaging::ActiveMessenger // Depends on active messenger sending
    >{}
  );

  p_->registerComponent<seq::Sequencer>(
    &theSeq, Deps<
      ctx::Context,              // Everything depends on theContext
      messaging::ActiveMessenger // Depends on active messenger for sequencing
    >{}
  );

  p_->registerComponent<seq::SequencerVirtual>(
    &theVirtualSeq, Deps<
      ctx::Context,               // Everything depends on theContext
      messaging::ActiveMessenger, // Depends on active messenger for sequencing
      vrt::VirtualContextManager  // Depends on virtual manager
    >{}
  );

  p_->registerComponent<location::LocationManager>(
    &theLocMan, Deps<
      ctx::Context,               // Everything depends on theContext
      messaging::ActiveMessenger  // Depends on active messenger for sending
    >{}
  );

  p_->registerComponent<vrt::VirtualContextManager>(
    &theVirtualManager, Deps<
      ctx::Context,               // Everything depends on theContext
      messaging::ActiveMessenger, // Depends on active messenger for messaging
      sched::Scheduler            // For scheduling work
    >{}
  );

  p_->registerComponent<vrt::collection::CollectionManager>(
    &theCollection, Deps<
      ctx::Context,                        // Everything depends on theContext
      messaging::ActiveMessenger,          // Depends on for messaging
      group::GroupManager,                 // For broadcasts
      sched::Scheduler,                    // For scheduling work
      location::LocationManager,           // For element location
      vrt::collection::balance::ProcStats, // For stat collection
      vrt::collection::balance::LBManager  // For load balancing
    >{}
  );

  p_->registerComponent<rdma::Manager>(
    &theHandleRDMA, Deps<
      ctx::Context,                       // Everything depends on theContext
      messaging::ActiveMessenger,         // Depends on active messenger for messaging
      vrt::collection::CollectionManager, // For RDMA on collection elements
      objgroup::ObjGroupManager,          // For RDMA on objgroups
      collective::CollectiveAlg           // Depends on collective scope
    >{}
  );

  p_->registerComponent<vrt::collection::balance::ProcStats>(
    &theProcStats, Deps<
      ctx::Context                        // Everything depends on theContext
    >{}
  );

  p_->registerComponent<vrt::collection::balance::StatsRestartReader>(
    &theStatsReader, Deps<
      ctx::Context,                        // Everything depends on theContext
      vrt::collection::balance::ProcStats  // Depends on proc stats for input
    >{}
  );

  p_->registerComponent<vrt::collection::balance::LBManager>(
    &theLBManager, Deps<
      ctx::Context,                        // Everything depends on theContext
      util::memory::MemoryUsage,           // Output mem usage on phase change
      vrt::collection::balance::ProcStats  // For stat collection
    >{}
  );

  p_->add<ctx::Context>();
  p_->add<util::memory::MemoryUsage>();
  p_->add<registry::Registry>();
  p_->add<event::AsyncEvent>();
  p_->add<pool::Pool>();
# if backend_check_enabled(trace_enabled)
  p_->add<trace::Trace>();
# endif
  p_->add<objgroup::ObjGroupManager>();
  p_->add<messaging::ActiveMessenger>();
  p_->add<sched::Scheduler>();
  p_->add<term::TerminationDetector>();
  p_->add<collective::CollectiveAlg>();
  p_->add<group::GroupManager>();
  p_->add<pipe::PipeManager>();
  p_->add<rdma::RDMAManager>();
  p_->add<param::Param>();
  p_->add<seq::Sequencer>();
  p_->add<seq::SequencerVirtual>();
  p_->add<location::LocationManager>();
  p_->add<vrt::VirtualContextManager>();
  p_->add<vrt::collection::CollectionManager>();
  p_->add<rdma::Manager>();
  p_->add<registry::Registry>();
  p_->add<event::AsyncEvent>();
  p_->add<pool::Pool>();
  p_->add<vrt::collection::balance::ProcStats>();
  p_->add<vrt::collection::balance::LBManager>();

  if (needStatsRestartReader()) {
    p_->add<vrt::collection::balance::StatsRestartReader>();
  }

  bool const has_workers = num_workers_ != no_workers;
  if (has_workers) {
    p_->add<worker::WorkerGroupType>();
  }

  p_->construct();

  if (communicator_ == MPI_COMM_NULL) {
    communicator_ = theContext->getComm();
  }

  debug_print(runtime, node, "end: initializeComponents\n");
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

}} //end namespace vt::runtime
