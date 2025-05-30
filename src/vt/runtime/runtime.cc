/*
//@HEADER
// *****************************************************************************
//
//                                  runtime.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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
#include "vt/messaging/active.h"
#include "vt/event/event.h"
#include "vt/termination/termination.h"
#include "vt/pool/pool.h"
#include "vt/rdma/rdma_headers.h"
#include "vt/pipe/pipe_manager.h"
#include "vt/objgroup/manager.h"
#include "vt/scheduler/scheduler.h"
#include "vt/topos/location/location_headers.h"
#include "vt/rdmahandle/manager.h"
#include "vt/vrt/context/context_vrtmanager.h"
#include "vt/vrt/collection/collection_headers.h"
#include "vt/vrt/collection/balance/lb_type.h"
#include "vt/configs/debug/debug_colorize.h"
#include "vt/configs/error/stack_out.h"
#include "vt/utils/memory/memory_usage.h"
#include "vt/runtime/component/component_pack.h"
#include "vt/utils/mpi_limits/mpi_max_tag.h"
#include "vt/vrt/collection/balance/lb_data_restart_reader.h"
#include "vt/timetrigger/time_trigger_manager.h"
#include "vt/phase/phase_manager.h"
#include "vt/epoch/epoch_manip.h"
#if vt_check_enabled(perf)
#include "vt/metrics/perf_data.h"
#endif

#include "vt/configs/arguments/app_config.h"
#include "vt/configs/arguments/args.h"

#include <checkpoint/checkpoint.h>

#include <stdexcept>
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
  int& argc, char**& argv, bool const interop_mode, MPI_Comm in_comm,
  RuntimeInstType const in_instance, arguments::AppConfig const* appConfig
)  : instance_(in_instance), runtime_active_(false), is_interop_(interop_mode),
     initial_communicator_(in_comm),
     arg_config_(std::make_unique<arguments::ArgConfig>()),
     app_config_(&arg_config_->config_)
{
  /// =========================================================================
  /// Notes on lifecycle for the ArgConfig/AppConfig
  /// =========================================================================
  ///
  /// - After \c vt::Runtime is constructed, the ArgConfig lives in
  ///   \c arg_config_ and \c app_config_ contains a pointer to the internals.
  ///
  /// - After the pack registers the ArgConfig component, \c arg_config_ is no
  ///   longer valid. The config is in a tuple awaiting construction---thus, we
  ///   can't easily access it. app_config_ remains valid during this time
  ///
  /// - After construction, the \c arg_config_ is in the component
  ///   \c theConfig() and can be accessed normally
  ///
  /// - After \c Runtime::finalize() is called but before the pack is destroyed,
  ///   we extract the \c ArgConfig from the component and put it back in
  ///   \c arg_config_ for use after.
  ///
  /// - From then on, until the \c vt::Runtime is destroyed or VT is
  ///   re-initialized \c arg_config_ will contain the configuration.
  ///
  ///  For this to all work correctly, the \c vt_debug_print infrastructure
  ///  calls \c vt::config::preConfig() which always grabs the correct app
  ///  config, from the component singleton or the \c vt::Runtime,
  ///  or provides stubbed arguments as a fallback.
  ///
  /// =========================================================================

  int prev_initialized;
  MPI_Initialized(&prev_initialized);

  if (not is_interop_) {
    vtAbortIf(
      prev_initialized, "MPI is already initialzed. Run VT under interop-mode?"
    );
    // Always initialize MPI first, before handling arguments.
    // This also allows MPI a first-stab at dealing with arguments.
    MPI_Init(&argc, &argv);
  } else {
    vtAbortIf(
      not prev_initialized, "MPI must be already initialized in VT interop-mode."
    );
  }

  // n.b. ref-update of args with pass-through arguments
  std::tuple<int, std::string> result =
    arg_config_->parse(argc, argv, appConfig);
  int exit_code = std::get<0>(result);

  if (getAppConfig()->vt_help_lb_args) {
    // Help requested or invalid argument(s).
    int rank = 0;
    MPI_Comm_rank(initial_communicator_, &rank);

    if (rank == 0) {
      // Help requested
      vt::debug::preConfigRef()->colorize_output = true;
      vrt::collection::balance::LBManager::printLBArgsHelp(getAppConfig()->vt_lb_name);
    }
    if (exit_code == -1) {
      exit_code = 0;
    }
  }

  if (exit_code not_eq -1) {
    // Help requested or invalid argument(s).
    MPI_Comm comm = initial_communicator_;

    int rank = 0;
    MPI_Comm_rank(comm, &rank);

    if (rank == 0) {
      // Only emit output to rank 0 to minimize spam
      std::string& msg = std::get<1>(result);
      // exit code of 0 -> 'help'
      std::ostream& out = exit_code == 0 ? std::cout : std::cerr;

      if (exit_code != 0) {
        out << "--- VT INITIALIZATION ABORT ---" << "\n";
      }
      out << "\n" << msg << "\n";
      if (exit_code != 0) {
        out << "--- VT INITIALIZATION ABORT ---" << "\n";
      }
      out << std::flush;
    }

    if (exit_code != 0) {
      // Even in interop mode, still abort MPI on bad args.
      MPI_Abort(comm, exit_code);
    }
    MPI_Finalize();

    std::_Exit(exit_code); // no return
  }

  sig_user_1_ = false;
  setupSignalHandler();
  setupSignalHandlerINT();
  setupTerminateHandler();

  if (arg_config_->config_.vt_lb_data) {
    determinePhysicalNodeIDs();
  }
}

void Runtime::determinePhysicalNodeIDs() {
  MPI_Comm i_comm = initial_communicator_;

  MPI_Comm shm_comm;
  MPI_Comm_split_type(i_comm, MPI_COMM_TYPE_SHARED, 0, MPI_INFO_NULL, &shm_comm);
  int shm_rank = -1;
  int node_size = -1;
  MPI_Comm_rank(shm_comm, &shm_rank);
  MPI_Comm_size(shm_comm, &node_size);

  int num_nodes = -1;
  int is_rank_0 = (shm_rank == 0) ? 1 : 0;
  MPI_Allreduce(&is_rank_0, &num_nodes, 1, MPI_INT, MPI_SUM, i_comm);

  int starting_rank = -1;
  MPI_Comm_rank(i_comm, &starting_rank);

  MPI_Comm node_number_comm;
  MPI_Comm_split(i_comm, shm_rank, starting_rank, &node_number_comm);

  int node_id = -1;
  if (shm_rank == 0) {
    MPI_Comm_rank(node_number_comm, &node_id);
  }
  MPI_Bcast(&node_id, 1, MPI_INT, 0, shm_comm);

  MPI_Comm_free(&shm_comm);
  MPI_Comm_free(&node_number_comm);

  has_physical_node_info = true;
  physical_node_id = node_id;
  physical_num_nodes = num_nodes;
  physical_node_size = node_size;
  physical_node_rank = shm_rank;
}

bool Runtime::hasSchedRun() const {
  return theSched ? theSched->hasSchedRun() : false;
}

void Runtime::pauseForDebugger() {
  if (theConfig()->vt_pause) {
    constexpr int max_buffer_length = 256;
    char node_str[max_buffer_length];
    auto node = vt::theContext() ? vt::theContext()->getNode() : -1;
    snprintf(node_str, max_buffer_length, "prog-%d.pid", node);
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
  handleSignalFailure();
}

/*static*/ void Runtime::sigHandlerUsr1([[maybe_unused]] int sig) {
  Runtime::sig_user_1_ = true;
}

/*static*/ void Runtime::sigHandler(int sig) {
  auto vt_pre    = debug::vtPre();
  auto bred      = debug::bred();
  ::fmt::print("{}Caught SIGSEGV signal: {} \n", vt_pre, sig);
  handleSignalFailure();
}

/*static*/ void Runtime::sigHandlerBus(int sig) {
  auto vt_pre    = debug::vtPre();
  auto bred      = debug::bred();
  ::fmt::print("{}Caught SIGBUS signal: {} \n", vt_pre, sig);
  handleSignalFailure();
}

/*static*/ void Runtime::termHandler() {
  auto vt_pre    = debug::vtPre();
  auto bred      = debug::bred();
  ::fmt::print("{}Caught std::terminate \n", vt_pre);
  handleSignalFailure();
}

/*static*/ void Runtime::handleSignalFailure() {
  // Tell all components there has been a fatal error
  if (rt->p_) {
    rt->p_->foreach([](component::BaseComponent* base) {
      base->fatalError();
    });
  }
  // Try to flush out all logs before dying
# if vt_check_enabled(trace_enabled)
  if (vt::theTrace()) {
    vt::theTrace()->cleanupTracesFile();
  }
# endif
  if (Runtime::nodeStackWrite()) {
    auto stack = debug::stack::dumpStack();
    auto stack_pretty = debug::stack::prettyPrintStack(stack);
    if (vt::theConfig()->vt_stack_file != "") {
      Runtime::writeToFile(stack_pretty);
    } else {
      ::fmt::print("{}", stack_pretty);
      ::fmt::print("\n");
    }
  }
  std::_Exit(EXIT_FAILURE);
}

/*static*/ bool Runtime::nodeStackWrite() {
  auto const& node = debug::preNode();
  if (node == uninitialized_destination) {
    return true;
  } else if (vt::theConfig()->vt_stack_mod == 0) {
    return true;
  } else if (node % vt::theConfig()->vt_stack_mod == 0) {
    return true;
  } else {
    return false;
  }
}

/*static*/ void Runtime::writeToFile(std::string const& str) {
  std::string& app_name = vt::theConfig()->prog_name;
  std::string name = vt::theConfig()->vt_stack_file == "" ? app_name : vt::theConfig()->vt_stack_file;
  auto const& node = debug::preNode();
  std::string file = name + "." + std::to_string(node) + ".stack.out";
  std::string dir  = vt::theConfig()->vt_stack_dir == "" ? "" : vt::theConfig()->vt_stack_dir + "/";
  std::string path = dir + file;
  FILE* f = fopen(path.c_str(), "w+");
  fprintf(f, "%s", str.c_str());
  fclose(f);
}

void Runtime::setupSignalHandler() {
  if (!arg_config_->config_.vt_no_sigsegv) {
    signal(SIGSEGV, Runtime::sigHandler);
  }
  if (!arg_config_->config_.vt_no_sigbus) {
    signal(SIGBUS, Runtime::sigHandlerBus);
  }
  signal(SIGUSR1, Runtime::sigHandlerUsr1);
}

void Runtime::setupSignalHandlerINT() {
  if (!arg_config_->config_.vt_no_sigint) {
    signal(SIGINT, Runtime::sigHandlerINT);
  }
}

void Runtime::setupTerminateHandler() {
  if (!arg_config_->config_.vt_no_terminate) {
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

  if (not is_interop_) {
    MPI_Finalize();
  }
}

bool Runtime::tryInitialize() {
  bool const init_now = !initialized_ && !finalized_;

  vt_debug_print_context(
    normal, runtime, unknown,
    "Runtime: tryInitialize: initialized_={}, finalized_={}, init_now={}\n",
    print_bool(initialized_), print_bool(finalized_), print_bool(init_now)
  );

  if (init_now) {
    initialize(true);
  }
  return init_now;
}

bool Runtime::tryFinalize(bool const disable_sig) {
  bool const rt_live = !finalized_ && initialized_;
  bool const has_run_sched = hasSchedRun();
  bool const finalize_now = rt_live && has_run_sched;

  vt_debug_print_context(
    normal, runtime, unknown,
    "Runtime: tryFinalize: initialized_={}, finalized_={}, rt_live={}, sched={}, "
    "finalize_now={}\n",
    print_bool(initialized_), print_bool(finalized_), print_bool(rt_live),
    print_bool(has_run_sched), print_bool(finalize_now)
  );

  if (finalize_now) {
    finalize(true, disable_sig);
  } else {
    finalize_on_term_ = true;
  }

  return finalize_now;
}

bool Runtime::needLBDataRestartReader() {
  using vrt::collection::balance::ReadLBConfig;
  using vrt::collection::balance::LBType;
  using vrt::collection::balance::get_lb_names;

  bool needOfflineLB = false;

  #if vt_check_enabled(lblite)
  if (ReadLBConfig::openConfig(arg_config_->config_.vt_lb_file_name)) {
    needOfflineLB = ReadLBConfig::hasOfflineLB();
  }

  needOfflineLB = needOfflineLB || arg_config_->config_.vt_lb_name == get_lb_names()[LBType::OfflineLB];

  if (needOfflineLB && !arg_config_->config_.vt_lb_data_in) {
    vtAbort("VT cannot run OfflineLB without '--vt_lb_data_in' parameter.");
  }
  #endif

  return needOfflineLB;
}

bool Runtime::initialize(bool const force_now) {
  if (force_now) {
    initializeComponents();
    initializeOptionalComponents();
    initializeErrorHandlers();

    MPI_Comm comm = theContext->getComm();

    MPI_Barrier(comm);
    if (theContext->getNode() == 0) {
      printStartupBanner();
      // Enqueue a check for later in case arguments are modified before work
      // actually executes
      theSched->enqueue([this]{
        this->checkForArgumentErrors();
      });

      // If the user specified to output a configuration file, write it to the
      // specified file on rank 0
      if (theConfig()->vt_output_config) {
        std::ofstream out(theConfig()->vt_output_config_file);
        out << theConfig()->vt_output_config_str;
        out.close();
      }
    }
    setup();

    // Runtime component is being re-initialized
    // Setup signal handlers based on AppConfig
    if (finalized_ && sig_handlers_disabled_) {
      setupSignalHandler();
      setupSignalHandlerINT();
      sig_handlers_disabled_ = false;
    }

    MPI_Barrier(comm);

    initialized_ = true;
    return true;
  } else {
    return tryInitialize();
  }
}

bool Runtime::finalize(bool const force_now, bool const disable_sig) {
  if (force_now) {
    using component::BaseComponent;

    MPI_Comm comm = theContext->getComm();

    auto const& is_zero = theContext->getNode() == 0;

#   if vt_check_enabled(diagnostics)
    if (getAppConfig()->vt_diag_enable) {
      computeAndPrintDiagnostics();
    }
#   endif

    if (getAppConfig()->vt_print_memory_footprint) {
      printMemoryFootprint();
    }

    auto const& num_units = theTerm->getNumUnits();
    auto const coll_epochs = theTerm->getNumTerminatedCollectiveEpochs();
    MPI_Barrier(comm);
    fflush(stdout);
    fflush(stderr);
    MPI_Barrier(comm);

    // Extract ArgConfig and keep it for use after VT is finalized
    arg_config_ = p_->extractComponent<vt::arguments::ArgConfig>("ArgConfig");

    // Context free's communicator in destructor; keep it around momentarily
    // so the communicator can be used for synchronization until the end.
    std::unique_ptr<vt::ctx::Context> context = p_->extractComponent<vt::ctx::Context>("Context");

    // Destroys and finalizes the remaining components in reverse initialization order
    p_.reset(nullptr);
    MPI_Barrier(comm);
    MPI_Barrier(comm);

    if (is_zero) {
      printShutdownBanner(num_units, coll_epochs);
    }

    if (disable_sig) {
      signal(SIGSEGV, SIG_DFL);
      signal(SIGUSR1, SIG_DFL);
      signal(SIGINT, SIG_DFL);
      sig_handlers_disabled_ = true;
    }

    MPI_Barrier(comm);

    theContext = nullptr; // used in some state checks
    context = nullptr;    // "use" to avoid warning

    finalized_ = true;
    return true;
  } else {
    return tryFinalize(disable_sig);
  }
}

void Runtime::systemSync() {
  // nb. suspected to be used outside of an initialized VT.
  MPI_Comm comm = theContext != nullptr
    ? theContext->getComm()
    : initial_communicator_;

  MPI_Barrier(comm);
}

void Runtime::reset() {
  MPI_Comm comm = theContext->getComm();
  MPI_Barrier(comm);

  runtime_active_ = true;

  auto action = std::bind(&Runtime::terminationHandler, this);
  theTerm->addDefaultAction(action);
  theTerm->resetGlobalTerm();

  MPI_Barrier(comm);

  // Without workers running on the node, the termination detector should
  // assume its locally ready to propagate instead of waiting for them to
  // become idle.
  theTerm->setLocalTerminated(true);
}

void Runtime::abort(std::string const abort_str, ErrorCodeType const code) {
  output(abort_str, code, true, true, false);

  if (theContext && theConfig()->vt_throw_on_abort) {
    throw std::runtime_error(abort_str);
  } else {
    aborted_ = true;
    std::raise(SIGTRAP);
    if (theContext) {
      auto const comm = theContext->getComm();
      MPI_Abort(comm, 129);
    } else {
      std::_Exit(code);
      // @todo: why will this not compile with clang!!?
      //quick_exit(code);
    }
  }
}

void Runtime::output(
  std::string const abort_str, [[maybe_unused]] ErrorCodeType const code,
  bool error, bool decorate, bool formatted
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
  auto separator = fmt::format("{}{}{:-^120}{}\n", prefix, bred, "", reset);
  auto warn_sep  = fmt::format("{}{}{:-^120}{}\n", prefix, byellow, "", reset);
  // auto space     = fmt::format("{}\n", prefix);

  if (decorate) {
    if (error) {
      auto f1 = fmt::format(" Runtime Error: System Aborting! ");
      auto const info = ::fmt::format(" Fatal Error on Node {} ", node);
      // fmt::print(stderr, "{}", space);
      fmt::print(stderr, "{}", separator);
      fmt::print(stderr, "{}{}{:-^120}{}\n", prefix, bred, f1, reset);
      fmt::print(stderr, "{}{}{:-^120}{}\n", prefix, bred, info, reset);
      fmt::print(stderr, "{}", separator);
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

  if (theContext == nullptr) {
    // Too early in init process to check dump settings - always dump stack.
    auto stack = debug::stack::dumpStack();
    auto stack_pretty = debug::stack::prettyPrintStack(stack);
    fmt::print("{}", stack_pretty);
  } else if (!theConfig()->vt_no_stack) {
    bool const on_abort = !theConfig()->vt_no_abort_stack;
    bool const on_warn = !theConfig()->vt_no_warn_stack;
    bool const dump = (error && on_abort) || (!error && on_warn);
    if (dump) {
      if (Runtime::nodeStackWrite()) {
        auto stack = debug::stack::dumpStack();
        auto stack_pretty = debug::stack::prettyPrintStack(stack);
        if (theConfig()->vt_stack_file != "") {
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
  vt_debug_print(
    terse, runtime,
    "Runtime: executing registered termination handler\n"
  );

  runtime_active_ = false;

  if (finalize_on_term_) {
    finalize();
  }
}

void Runtime::setup() {
  vt_debug_print(normal, runtime, "begin: setup\n");

  runtime_active_ = true;

  auto action = std::bind(&Runtime::terminationHandler, this);
  theTerm->addDefaultAction(action);

  // wait for all nodes to start up to initialize the runtime
  theCollective->barrier();

# if vt_check_enabled(trace_enabled)
  theTrace->loadAndBroadcastSpec();
# endif
  theNodeLBData->loadAndBroadcastSpec();

  if (theConfig()->vt_pause) {
    pauseForDebugger();
  }

  vt_debug_print(normal, runtime, "end: setup\n");
}

void Runtime::initializeComponents() {
  vt_debug_print(normal, runtime, "begin: initializeComponents\n");

  using component::ComponentPack;
  using component::StartupDeps;
  using component::RuntimeDeps;

  p_ = std::make_unique<ComponentPack>();
  bool addLBDataRestartReader = needLBDataRestartReader();
# if vt_check_enabled(trace_enabled)
  std::string const prog_name = arg_config_->config_.prog_name;
# endif

  p_->registerComponent<arguments::ArgConfig>(
    &theArgConfig,
    StartupDeps<>{},
    RuntimeDeps<>{},
    std::move(arg_config_)
  );

  p_->registerComponent<ctx::Context>(
    &theContext,
    StartupDeps<arguments::ArgConfig>{},
    RuntimeDeps<>{},
    is_interop_, initial_communicator_
  );

  p_->registerComponent<util::memory::MemoryUsage>(&theMemUsage,
    StartupDeps<
      ctx::Context,       // Everything depends on theContext
      phase::PhaseManager // For outputting memory at phase boundaries
    >{},
    RuntimeDeps<>{}
  );

  p_->registerComponent<pool::Pool>(
    &thePool,
    StartupDeps<
      ctx::Context // Everything depends on theContext
    >{},
    RuntimeDeps<>{}
  );

  p_->registerComponent<event::AsyncEvent>(
    &theEvent,
    StartupDeps<
#     if vt_check_enabled(trace_enabled)
      trace::Trace,  // For trace user event registrations
#     endif
      ctx::Context,   // Everything depends on theContext
      pool::Pool     // For memory allocations
    >{},
    RuntimeDeps<>{}
  );

# if vt_check_enabled(trace_enabled)
  // The Trace and Scheduler components have a co-dependency. However,
  // the lifetime of theTrace should be longer than that of theSched.
  p_->registerComponent<trace::Trace>(
    &theTrace,
    StartupDeps<
      ctx::Context,              // Everything depends on theContext
      objgroup::ObjGroupManager  // Depends on objgroup for construction
    >{},
    RuntimeDeps<>{},
    prog_name
  );
# endif

# if vt_check_enabled(mpi_access_guards)
  p_->registerComponent<pmpi::PMPIComponent>(
    &thePMPI,
    StartupDeps<
#     if vt_check_enabled(trace_enabled)
      trace::Trace,  // For PMPI tracing, if tracing is enabled.
#     endif
      ctx::Context   // Everything depends on theContext
    >{},
    RuntimeDeps<>{}
  );
#endif

  p_->registerComponent<objgroup::ObjGroupManager>(
    &theObjGroup,
    StartupDeps<
      ctx::Context,               // Everything depends on theContext
      pool::Pool                  // For memory allocations
    >{},
    RuntimeDeps<
      messaging::ActiveMessenger // Depends on active messenger to send
    >{}
  );

  p_->registerComponent<messaging::ActiveMessenger>(
    &theMsg,
    StartupDeps<
#     if vt_check_enabled(trace_enabled)
      trace::Trace,             // For trace user event registrations
#     endif
      ctx::Context,             // Everything depends on theContext
      pool::Pool                // Depends on pool for message allocation
    >{},
    RuntimeDeps<
      phase::PhaseManager, // For data collection at phase boundaries
      event::AsyncEvent,   // Depends on event to send messages
      sched::Scheduler     // For scheduling work through runnables
    >{}
  );

  p_->registerComponent<sched::Scheduler>(
    &theSched,
    StartupDeps<
#     if vt_check_enabled(trace_enabled)
      trace::Trace,             // For scheduler-related trace events
#     endif
      ctx::Context              // Everything depends on theContext
    >{},
    RuntimeDeps<
      util::memory::MemoryUsage // Depends on memory usage for output
    >{}
  );

  p_->registerComponent<epoch::EpochManip>(
    &theEpoch,
    StartupDeps<
      ctx::Context                // Everything depends on theContext
    >{},
    RuntimeDeps<>{}
  );

  p_->registerComponent<term::TerminationDetector>(
    &theTerm,
    StartupDeps<
      ctx::Context,               // Everything depends on theContext
      epoch::EpochManip           // Depends on for generating epochs
    >{},
    RuntimeDeps<
      messaging::ActiveMessenger, // Depends on active messenger to send term msgs
      sched::Scheduler            // Depends on scheduler for idle checks
    >{}
  );

  p_->registerComponent<collective::CollectiveAlg>(
    &theCollective,
    StartupDeps<
      ctx::Context               // Everything depends on theContext
    >{},
    RuntimeDeps<
      messaging::ActiveMessenger // Depends on active messenger for collectives
    >{}
  );

  p_->registerComponent<group::GroupManager>(
    &theGroup,
    StartupDeps<
      ctx::Context,               // Everything depends on theContext
      messaging::ActiveMessenger, // Depends on active messenger for setting up
      collective::CollectiveAlg,  // Depends on collective for spanning trees
      sched::Scheduler,           // Depends on scheduler for starting up
      pool::Pool,                 // Depends on pool for message allocation
      term::TerminationDetector,  // Need TD to determine completion
      event::AsyncEvent           // Depends on event to send messages
    >{},
    RuntimeDeps<>{}
  );

  p_->registerComponent<pipe::PipeManager>(
    &theCB,
    StartupDeps<
      ctx::Context,                        // Everything depends on theContext
      group::GroupManager                  // For creating a new group
    >{},
    RuntimeDeps<
      messaging::ActiveMessenger,          // Depends on AM for callbacks
      collective::CollectiveAlg,           // Depends on collective for callbacks
      objgroup::ObjGroupManager,           // Depends on objgroup for callbacks
      vrt::collection::CollectionManager   // Depends collection for callbacks
    >{}
  );

  p_->registerComponent<rdma::RDMAManager>(
    &theRDMA,
    StartupDeps<
      ctx::Context,                 // Everything depends on theContext
      collective::CollectiveAlg     // Depends on collective scope
    >{},
    RuntimeDeps<
      messaging::ActiveMessenger    // Depends on active messenger for RDMA
    >{}
  );

  p_->registerComponent<location::LocationManager>(
    &theLocMan,
    StartupDeps<
      ctx::Context                // Everything depends on theContext
    >{},
    RuntimeDeps<
      messaging::ActiveMessenger,  // Depends on active messenger for sending
      objgroup::ObjGroupManager   // Depends on objgroup since it creates them
    >{}
  );

  p_->registerComponent<vrt::VirtualContextManager>(
    &theVirtualManager,
    StartupDeps<
      ctx::Context                // Everything depends on theContext
    >{},
    RuntimeDeps<
      messaging::ActiveMessenger, // Depends on active messenger for messaging
      sched::Scheduler            // For scheduling work
    >{}
  );

  p_->registerComponent<vrt::collection::CollectionManager>(
    &theCollection,
    StartupDeps<
      ctx::Context,             // Everything depends on theContext
      location::LocationManager // For element location
    >{},
    RuntimeDeps<
      messaging::ActiveMessenger,           // Depends on for messaging
      group::GroupManager,                  // For broadcasts
      sched::Scheduler,                     // For scheduling work
      objgroup::ObjGroupManager,            // Depends on objgroup
      pipe::PipeManager,                    // Depends on using callbacks
      vrt::collection::balance::NodeLBData, // For LB data collection
      vrt::collection::balance::LBManager   // For load balancing
    >{}
  );

  p_->registerComponent<rdma::Manager>(
    &theHandleRDMA,
    StartupDeps<
      ctx::Context,                        // Everything depends on theContext
      collective::CollectiveAlg,           // Depends on collective scope
      objgroup::ObjGroupManager            // Since it's an objgroup
    >{},
    RuntimeDeps<
      messaging::ActiveMessenger,         // Depends on active messenger for messaging
      vrt::collection::CollectionManager  // For RDMA on collection elements
    >{}
  );

  p_->registerComponent<vrt::collection::balance::NodeLBData>(
    &theNodeLBData,
    StartupDeps<
      ctx::Context,                        // Everything depends on theContext
      objgroup::ObjGroupManager            // Since it's an objgroup
    >{},
    RuntimeDeps<
      phase::PhaseManager                 // For phase structure
    >{}
  );

  p_->registerComponent<vrt::collection::balance::LBDataRestartReader>(
    &theLBDataReader,
    StartupDeps<
      ctx::Context,                          // Everything depends on theContext
      objgroup::ObjGroupManager              // Since it's an objgroup
    >{},
    RuntimeDeps<
      vrt::collection::balance::NodeLBData   // Depends on node LB data for input
    >{}
  );

  p_->registerComponent<vrt::collection::balance::LBManager>(
    &theLBManager,
    StartupDeps<
      ctx::Context,                         // Everything depends on theContext
      objgroup::ObjGroupManager,            // Since it's an objgroup
      vrt::collection::balance::NodeLBData  // For LB data collection
    >{},
    RuntimeDeps<
      util::memory::MemoryUsage,           // Output mem usage on phase change
      phase::PhaseManager                  // For phase structure
    >{}
  );

  p_->registerComponent<timetrigger::TimeTriggerManager>(
    &theTimeTrigger,
    StartupDeps<
      ctx::Context                         // Everything depends on theContext
    >{},
    RuntimeDeps<>{}
  );

  p_->registerComponent<phase::PhaseManager>(
    &thePhase,
    StartupDeps<
      ctx::Context,                        // Everything depends on theContext
      objgroup::ObjGroupManager            // Since it's an objgroup
    >{},
    RuntimeDeps<>{}
  );

# if vt_check_enabled(perf)
  p_->registerComponent<metrics::PerfData>(
    &thePerfData,
    StartupDeps<
      ctx::Context                        // Everything depends on theContext
    >{},
    RuntimeDeps<>{}
  );
# endif

  p_->add<arguments::ArgConfig>();
  p_->add<ctx::Context>();
  p_->add<util::memory::MemoryUsage>();
  p_->add<event::AsyncEvent>();
  p_->add<pool::Pool>();
# if vt_check_enabled(trace_enabled)
  p_->add<trace::Trace>();
# endif
# if vt_check_enabled(mpi_access_guards)
  p_->add<pmpi::PMPIComponent>();
# endif
  p_->add<objgroup::ObjGroupManager>();
  p_->add<messaging::ActiveMessenger>();
  p_->add<sched::Scheduler>();
  p_->add<term::TerminationDetector>();
  p_->add<collective::CollectiveAlg>();
  p_->add<group::GroupManager>();
  p_->add<pipe::PipeManager>();
  p_->add<rdma::RDMAManager>();
  p_->add<location::LocationManager>();
  p_->add<vrt::VirtualContextManager>();
  p_->add<vrt::collection::CollectionManager>();
  p_->add<rdma::Manager>();
  p_->add<event::AsyncEvent>();
  p_->add<pool::Pool>();
  p_->add<vrt::collection::balance::NodeLBData>();
  p_->add<vrt::collection::balance::LBManager>();
  p_->add<timetrigger::TimeTriggerManager>();
  p_->add<phase::PhaseManager>();
# if vt_check_enabled(perf)
  p_->add<metrics::PerfData>();
# endif

  if (addLBDataRestartReader) {
    p_->add<vrt::collection::balance::LBDataRestartReader>();
  }

  p_->construct();

  vt_debug_print(
    normal, runtime,
    "end: initializeComponents\n"
  );
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
  void mpiErrorHandler([[maybe_unused]] MPI_Comm *comm, int *errc, ...) {
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
  vt_debug_print(
    verbose, runtime,
    "begin: initializeOptionalComponents\n"
  );

  initializeTDCallbacks();

  vt_debug_print(
    verbose, runtime,
    "end: initializeOptionalComponents\n"
  );
}

void Runtime::initializeTDCallbacks() {
  using ::vt::ctx::ContextAttorney;

  vt_debug_print(
    normal, runtime,
    "begin: initializeTDCallbacks\n"
  );

  // Without workers running on the node, the termination detector should
  // enable/disable the global collective epoch based on the state of the
  // scheduler; register listeners to activate/deactivate that epoch
  auto td = vt::theTerm();
  theSched->registerTrigger(
    sched::SchedulerEvent::BeginIdleMinusTerm, [td]{
      vt_debug_print(
        normal, runtime,
        "setLocalTerminated: BeginIdle: true\n"
      );
      td->setLocalTerminated(true, false);
    }
  );
  theSched->registerTrigger(
    sched::SchedulerEvent::EndIdleMinusTerm, [td]{
      vt_debug_print(
        normal, runtime,
        "setLocalTerminated: EndIdle: false\n"
      );
      td->setLocalTerminated(false, false);
    }
  );

  vt_debug_print(
    normal, runtime,
    "end: initializeTDCallbacks\n"
  );
}

template<typename T>
void printComponentFootprint(T* component) {
  if (component != nullptr) {
    auto bytes = checkpoint::getMemoryFootprint(
      *component,
      component->getDiagnosticsFootprint()
    );
    auto const ret = util::memory::getBestMemoryUnit(bytes);
    auto const new_value = fmt::format("{:.{}f}", std::get<1>(ret), 1);
    auto const unit = std::get<0>(ret);

    fmt::print(
      "{}{}\t{:<24}{}{:>6} {}{}\n",
      debug::vtPre(),
      debug::reset(),
      component->name(),
      debug::magenta(),
      new_value,
      unit,
      debug::reset()
    );
  }
}

void Runtime::printMemoryFootprint() const {
  fmt::print(
    "{}{}Printing memory footprint for live components:{}\n",
    debug::vtPre(),
    debug::green(),
    debug::reset()
  );

  p_->foreach([&](component::BaseComponent* base) {
    auto name = base->name();
    if (name == "ArgConfig") {
      printComponentFootprint(
        static_cast<arguments::ArgConfig*>(base)
      );
    } else if (name == "Context") {
      printComponentFootprint(
        static_cast<ctx::Context*>(base)
      );
    } else if (name == "MemoryPool") {
      printComponentFootprint(
        static_cast<pool::Pool*>(base)
      );
    } else if (name == "VirtualContextManager") {
      printComponentFootprint(
        static_cast<vrt::VirtualContextManager*>(base)
      );
    } else if (name == "Collective") {
      printComponentFootprint(
        static_cast<collective::CollectiveAlg*>(base)
      );
    } else if (name == "AsyncEvent") {
      printComponentFootprint(
        static_cast<event::AsyncEvent*>(base)
      );
    } else if (name == "ActiveMessenger") {
      printComponentFootprint(
        static_cast<messaging::ActiveMessenger*>(base)
      );
    } else if (name == "RDMAManager") {
      printComponentFootprint(
        static_cast<rdma::RDMAManager*>(base)
      );
    } else if (name == "Scheduler") {
      printComponentFootprint(
        static_cast<sched::Scheduler*>(base)
      );
    } else if (name == "TerminationDetector") {
      printComponentFootprint(
        static_cast<term::TerminationDetector*>(base)
      );
    } else if (name == "LocationManager") {
      printComponentFootprint(
        static_cast<location::LocationManager*>(base)
      );
    } else if (name == "CollectionManager") {
      printComponentFootprint(
        static_cast<vrt::collection::CollectionManager*>(base)
      );
    } else if (name == "GroupManager") {
      printComponentFootprint(
        static_cast<group::GroupManager*>(base)
      );
    } else if (name == "PipeManager") {
      printComponentFootprint(
        static_cast<pipe::PipeManager*>(base)
      );
    } else if (name == "ObjGroupManager") {
      printComponentFootprint(
        static_cast<objgroup::ObjGroupManager*>(base)
      );
    } else if (name == "HandleRDMA") {
      printComponentFootprint(
        static_cast<rdma::Manager*>(base)
      );
    } else if (name == "MemoryUsage") {
      printComponentFootprint(
        static_cast<util::memory::MemoryUsage*>(base)
      );
    } else if (name == "NodeLBData") {
      printComponentFootprint(
        static_cast<vrt::collection::balance::NodeLBData*>(base)
      );
    } else if (name == "LBDataRestartReader") {
      printComponentFootprint(
        static_cast<vrt::collection::balance::LBDataRestartReader*>(base)
      );
    } else if (name == "LBManager") {
      printComponentFootprint(
        static_cast<vrt::collection::balance::LBManager*>(base)
      );
    } else if (name == "TimeTriggerManager") {
      printComponentFootprint(
        static_cast<timetrigger::TimeTriggerManager*>(base)
      );
    } else if (name == "PhaseManager") {
      printComponentFootprint(
        static_cast<vt::phase::PhaseManager*>(base)
      );
    } else if (name == "EpochManip") {
      printComponentFootprint(
        static_cast<vt::epoch::EpochManip*>(base)
      );
    #if vt_check_enabled(trace_enabled)
    } else if (name == "Trace") {
      printComponentFootprint(
        static_cast<trace::Trace*>(base)
      );
    #endif
    #if vt_check_enabled(mpi_access_guards)
    } else if (name == "PMPI") {
      printComponentFootprint(
        static_cast<pmpi::PMPIComponent*>(base)
      );
    #endif
    } else {
      fmt::print(
        "{}{}\tWarning:{} memory footprinting is not supported for {}!{}\n",
        debug::vtPre(),
        debug::red(),
        debug::reset(),
        name,
        debug::reset()
      );
    }
  });
}

arguments::AppConfig const* Runtime::getAppConfig() const {
  return app_config_;
}

}} //end namespace vt::runtime
