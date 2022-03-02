/*
//@HEADER
// *****************************************************************************
//
//                                  runtime.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_RUNTIME_RUNTIME_H
#define INCLUDED_VT_RUNTIME_RUNTIME_H

#include "vt/config.h"
#include "vt/runtime/runtime_common.h"
#include "vt/runtime/runtime_component_fwd.h"
#include "vt/worker/worker_headers.h"

// Optional components
#if vt_check_enabled(trace_enabled)
#include "vt/trace/trace.h"
#endif
#include "vt/pmpi/pmpi_component.h"

#include <memory>
#include <functional>
#include <string>

#include <mpi.h>

namespace vt { namespace runtime {

/**
 * \struct Runtime
 *
 * \brief The VT runtime that manages startup/shutdown and pointers to all the
 * live components for a runtime instance.
 *
 * The runtime contains an instance of VT that is initialized/finalized. It
 * performs the sequence of operations for correct initialization and
 * finalization of all the components that make up a runtime.
 *
 * The runtime can be configured to catch signals when errors occur and dump a
 * stack trace.
 */
struct Runtime {
  template <typename ComponentT>
  using ComponentPtrType = ComponentT*;

  /**
   * \internal \brief Initialize a VT runtime
   *
   * Under interop mode, MPI is not initialized or finalized by the runtime.
   * This can be used to embed VT into a larger context.
   *
   * When not running in interop mode, MPI is initialized in the constructor
   * and finalized in the destructor.
   *
   * \param[in] argc argument count (modified after VT extracts)
   * \param[in] argv arguments  (modified after VT extracts)
   * \param[in] in_num_workers number of worker threads to initialize
   * \param[in] interop_mode whether running in interoperability mode
   * \param[in] in_comm the MPI communicator (if in interoperability mode)
   * \param[in] in_instance the runtime instance to set
   */
  Runtime(
    int& argc, char**& argv,
    WorkerCountType in_num_workers = no_workers,
    bool const interop_mode = false,
    MPI_Comm in_comm = MPI_COMM_WORLD,
    RuntimeInstType const in_instance = RuntimeInstType::DefaultInstance,
    arguments::AppConfig const* appConfig = nullptr
  );

  Runtime(Runtime const&) = delete;
  Runtime(Runtime&&) = delete;
  Runtime& operator=(Runtime const&) = delete;

  virtual ~Runtime();

  /**
   * \brief Check if runtime is live
   *
   * \return has been initialized and not subsequently finalized
   */
  bool isLive() const { return p_ and p_->isLive(); }

  /**
   * \brief Invoke all the progress functions
   *
   * \return returns an unspecified value
   */
  int progress() { if (p_) return p_->progress(); else return 0; }

  /**
   * \brief Check if runtime has terminated
   *
   * \return whether it has terminated
   */
  bool isTerminated() const { return not runtime_active_; }

  /**
   * \brief Check if runtime is finalizable
   *
   * \return whether it is finalizable
   */
  bool isFinalizable() const { return initialized_ and not finalized_; }

  /**
   * \brief Check if runtime is initialized
   *
   * \return whether it is initialized
   */
  bool isInitialized() const { return initialized_; }

  /**
   * \brief Check if runtime is finalized
   *
   * \return whether it is finalized
   */
  bool isFinalized() const { return finalized_; }

  /**
   * \brief Check if scheduler has run
   *
   * \return whether it has run
   */
  bool hasSchedRun() const;

  /**
   * \brief Reset the runtime after termination is reached to use it again
   */
  void reset();

  /**
   * \internal \brief Initialize the runtime
   *
   * \param[in] force_now whether to force initialization regardless of state
   *
   * \return whether it initialized or not
   */
  bool initialize(bool const force_now = false);

  /**
   * \internal \brief Finalize the runtime
   *
   * \param[in] force_now whether to force finalization regardless of state
   * \param[in] disable_sig whether to disable signal handlers
   *
   * \return whether it finalized or not
   */
  bool finalize(bool const force_now = false, bool const disable_sig = true);

  /**
   * \internal \brief Compute the diagnostics across the components and print
   * them if requested at the end of the program
   */
  void computeAndPrintDiagnostics();

  /**
   * \brief Abort--die immediately after spitting out error message
   *
   * \param[in] abort_str the error message
   * \param[in] code the error code to output
   */
  void abort(std::string const abort_str, ErrorCodeType const code);

  /**
   * \brief Output a message and possibly die
   *
   * \param[in] abort_str the message
   * \param[in] code the code to throw
   * \param[in] error whether it's a fatal error
   * \param[in] decorate whether to decorate the message
   * \param[in] formatted whether it's already formatted or not
   */
  void output(
    std::string const abort_str, ErrorCodeType const code, bool error = false,
    bool decorate = true, bool formatted = true
  );

  /**
   * \internal \brief Get runtime instance ID
   *
   * \return the instance ID
   */
  RuntimeInstType getInstanceID() const { return instance_; }

  /**
   * \internal \brief Do a sync
   *
   * \warning Do not call this. It does an unsafe \c MPI_Barrier
   *
   * \todo Remove this and fix the single one callsite in \c NodeStats
   */
  void systemSync();

public:
  /**
   * \internal \brief Check for input argument errors
   */
  void checkForArgumentErrors();

  /**
   * \internal \brief Print memory footprint for all live components
   */
  void printMemoryFootprint() const;

private:
  RuntimeInstType const instance_;

  /**
   * \internal \brief Handle unexpected termination
   */
  static void handleSignalFailure();

  /**
   * \internal \brief Check if this node should dump during stack output
   *
   * \return whether this node is allowed to write
   */
  static bool nodeStackWrite();

  /**
   * \internal \brief Write a stack trace out of a file
   *
   * \param[in] str the stack to output to file
   */
  static void writeToFile(std::string const& str);

protected:
  /**
   * \internal \brief Try to initialize
   *
   * \return whether it succeeded
   */
  bool tryInitialize();

  /**
   * \internal \brief Try to finalize
   * \param[in] disable_sig whether to disable signal handlers
   *
   * \return whether it succeeded
   */
  bool tryFinalize(bool const disable_sig = true);

  /**
   * \internal \brief Initialize error handlers
   */
  void initializeErrorHandlers();

  /**
   * \internal \brief Initialize all the VT components
   */
  void initializeComponents();

  /**
   * \internal \brief Initialize optional components, like workers
   */
  void initializeOptionalComponents();

  /**
   * \internal \brief Initialize workers
   *
   * \param[in] num_workers number of workers to create
   */
  void initializeWorkers(WorkerCountType const num_workers);

  /**
   * \internal \brief Check if we should create a stats restart reader component
   *
   * \return whether we should create it
   */
  bool needStatsRestartReader();

  /**
   * \internal \brief Perform setup actions, such as registering a termination
   * detector action for detecting global termination
   */
  void setup();

  /**
   * \internal \brief Handler when global termination is reached
   */
  void terminationHandler();

  /**
   * \internal \brief Print a very informative startup banner
   */
  void printStartupBanner();

  /**
   * \internal \brief Print the shutdown banner
   *
   * \param[in] num_units total number of units processed
   * \param[in] coll_epochs total number of collective epochs processed
   */
  void printShutdownBanner(
    term::TermCounterType const& num_units, std::size_t const coll_epochs
  );

  /**
   * \internal \brief Pause for debugger at startup
   */
  void pauseForDebugger();

  /**
   * \internal \brief Setup the handlers for SIGSEGV, SIGBUS and SIGUSR1
   */
  void setupSignalHandler();

  /**
   * \internal \brief Setup the SIGINT signal handler
   */
  void setupSignalHandlerINT();

  /**
   * \internal \brief Setup the std::terminate handler
   */
  void setupTerminateHandler();

  /**
   * \internal \brief SIGSEGV signal handler
   */
  static void sigHandler(int sig);

  /**
   * \internal \brief SIGBUS signal handler
   */
  static void sigHandlerBus(int sig);

  /**
   * \internal \brief SIGUSR1 signal handler
   */
  static void sigHandlerUsr1(int sig);

  /**
   * \internal \brief SIGINT signal handler
   */
  static void sigHandlerINT(int sig);

  /**
   * \internal \brief std::terminate handler
   */
  static void termHandler();

public:
  /**
   * \brief Get the app config before the runtime has setup all the components
   * and VT is fully initialized.
   *
   * \note This should always return a valid pointer if the runtime is
   * constructed.
   *
   * \return the app config
   */
  arguments::AppConfig const* getAppConfig() const;

public:
  ComponentPtrType<arguments::ArgConfig> theArgConfig = nullptr;
  ComponentPtrType<registry::Registry> theRegistry = nullptr;
  ComponentPtrType<messaging::ActiveMessenger> theMsg = nullptr;
  ComponentPtrType<ctx::Context> theContext = nullptr;
  ComponentPtrType<event::AsyncEvent> theEvent = nullptr;
  ComponentPtrType<term::TerminationDetector> theTerm = nullptr;
  ComponentPtrType<collective::CollectiveAlg> theCollective = nullptr;
  ComponentPtrType<pool::Pool> thePool = nullptr;
  ComponentPtrType<rdma::RDMAManager> theRDMA = nullptr;
  ComponentPtrType<param::Param> theParam = nullptr;
  ComponentPtrType<seq::Sequencer> theSeq = nullptr;
  ComponentPtrType<seq::SequencerVirtual> theVirtualSeq = nullptr;
  ComponentPtrType<sched::Scheduler> theSched = nullptr;
  ComponentPtrType<location::LocationManager> theLocMan = nullptr;
  ComponentPtrType<vrt::VirtualContextManager> theVirtualManager = nullptr;
  ComponentPtrType<vrt::collection::CollectionManager> theCollection = nullptr;
  ComponentPtrType<group::GroupManager> theGroup = nullptr;
  ComponentPtrType<pipe::PipeManager> theCB = nullptr;
  ComponentPtrType<objgroup::ObjGroupManager> theObjGroup = nullptr;
  ComponentPtrType<util::memory::MemoryUsage> theMemUsage = nullptr;
  ComponentPtrType<rdma::Manager> theHandleRDMA = nullptr;
  ComponentPtrType<vrt::collection::balance::NodeStats> theNodeStats = nullptr;
  ComponentPtrType<vrt::collection::balance::StatsRestartReader> theStatsReader = nullptr;
  ComponentPtrType<vrt::collection::balance::LBManager> theLBManager = nullptr;
  ComponentPtrType<timetrigger::TimeTriggerManager> theTimeTrigger = nullptr;
  ComponentPtrType<phase::PhaseManager> thePhase = nullptr;
  ComponentPtrType<epoch::EpochManip> theEpoch = nullptr;

  // Node-level worker-based components for vt (these are optional)
  #if vt_threading_enabled
  ComponentPtrType<worker::WorkerGroupType> theWorkerGrp = nullptr;
  #endif

  // Optional components
  #if vt_check_enabled(trace_enabled)
    ComponentPtrType<trace::Trace> theTrace = nullptr;
  #endif
  #if vt_check_enabled(mpi_access_guards)
    ComponentPtrType<pmpi::PMPIComponent> thePMPI = nullptr;
  #endif

  static bool volatile sig_user_1_;

protected:
  bool finalize_on_term_ = false;
  bool initialized_ = false, finalized_ = false, aborted_ = false;
  bool runtime_active_ = false;
  bool is_interop_ = false;
  bool sig_handlers_disabled_ = false;
  WorkerCountType num_workers_ = no_workers;
  //< Communicator to be given to theContext creation; don't use otherwise.
  MPI_Comm initial_communicator_ = MPI_COMM_NULL;
  std::unique_ptr<component::ComponentPack> p_;
  std::unique_ptr<arguments::ArgConfig> arg_config_;
  arguments::AppConfig const* app_config_;   /**< App config during startup */
};

}} /* end namespace vt::runtime */

#endif /*INCLUDED_VT_RUNTIME_RUNTIME_H*/
