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
#include "vt/vrt/collection/balance/lb_type.h"
#include "vt/vrt/collection/collection_headers.h"
#include "vt/worker/worker_headers.h"
#include "vt/configs/generated/vt_git_revision.h"
#include "vt/configs/debug/debug_colorize.h"
#include "vt/configs/arguments/args.h"
#include "vt/configs/error/stack_out.h"
#include "vt/configs/error/pretty_print_stack.h"
#include "vt/utils/memory/memory_usage.h"

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

void Runtime::initializeLB() {
  #if backend_check_enabled(lblite)
    if (ArgType::vt_lb_stats) {
      auto lbNames = vrt::collection::balance::lb_names_;
      auto mapLB = vrt::collection::balance::LBType::StatsMapLB;
      if (ArgType::vt_lb_name == lbNames[mapLB]) {
        auto const node = theContext->getNode();
        const std::string &base_file = ArgType::vt_lb_stats_file_in;
        const std::string &dir = ArgType::vt_lb_stats_dir_in;
        auto const file = fmt::format("{}.{}.out", base_file, node);
        const auto file_name =
        static_cast<std::string>(fmt::format("{}/{}", dir, file));
        vrt::collection::balance::ProcStats::readRestartInfo(file_name);
      }
    }
  #endif
}

bool Runtime::initialize(bool const force_now) {
  if (force_now) {
    initializeContext(user_argc_, user_argv_, communicator_);
    initializeComponents();
    initializeOptionalComponents();
    initializeErrorHandlers();
    initializeLB();
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

  MPI_Barrier(theContext->getComm());

  // wait for all nodes to start up to initialize the runtime
  theCollective->barrierThen([this]{
    MPI_Barrier(theContext->getComm());
  });

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
  theHandleRDMA = rdma::Manager::construct().get();

  #if backend_check_enabled(trace_enabled)
    theTrace->initialize();
  #endif

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
  theHandleRDMA->destroy(); theHandleRDMA = nullptr;
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
