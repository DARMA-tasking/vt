/*
//@HEADER
// *****************************************************************************
//
//                                  runtime.h
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

#if !defined INCLUDED_RUNTIME_H
#define INCLUDED_RUNTIME_H

#include "vt/config.h"
#include "vt/runtime/runtime_common.h"
#include "vt/runtime/runtime_component_fwd.h"
#include "vt/worker/worker_headers.h"
#include "vt/configs/arguments/args.h"

#if vt_check_enabled(trace_enabled)
#include "vt/trace/trace.h"
#endif

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
  using ArgType = vt::arguments::ArgConfig;

  /**
   * \internal \brief Initialize a VT runtime
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
    WorkerCountType in_num_workers = no_workers, bool const interop_mode = false,
    MPI_Comm* in_comm = nullptr,
    RuntimeInstType const in_instance = RuntimeInstType::DefaultInstance
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
   *
   * \return whether it finalized or not
   */
  bool finalize(bool const force_now = false);

  /**
   * \brief Run the scheduler once
   */
  void runScheduler();

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
   * \todo Remove this and fix the single one callsite in \c ProcStats
   */
  void systemSync() { sync(); }

public:
  /**
   * \internal \brief Check for input argument errors
   */
  void checkForArgumentErrors();

private:
  RuntimeInstType const instance_;

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
   *
   * \return whether it succeeded
   */
  bool tryFinalize();

  /**
   * \internal \brief Setup argument input
   */
  void setupArgs();

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
   * \internal \brief Finalize MPI
   */
  void finalizeMPI();

  /**
   * \internal \brief Perform a synchronization during startup/shutdown using an
   * \c MPI_Barrier
   */
  void sync();

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
   * \internal \brief Setup the SIGSEGV and SIGUSR1 signal handler
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
  ComponentPtrType<registry::Registry> theRegistry;
  ComponentPtrType<messaging::ActiveMessenger> theMsg;
  ComponentPtrType<ctx::Context> theContext;
  ComponentPtrType<event::AsyncEvent> theEvent;
  ComponentPtrType<term::TerminationDetector> theTerm;
  ComponentPtrType<collective::CollectiveAlg> theCollective;
  ComponentPtrType<pool::Pool> thePool;
  ComponentPtrType<rdma::RDMAManager> theRDMA;
  ComponentPtrType<param::Param> theParam;
  ComponentPtrType<seq::Sequencer> theSeq;
  ComponentPtrType<seq::SequencerVirtual> theVirtualSeq;
  ComponentPtrType<sched::Scheduler> theSched;
  ComponentPtrType<location::LocationManager> theLocMan;
  ComponentPtrType<vrt::VirtualContextManager> theVirtualManager;
  ComponentPtrType<vrt::collection::CollectionManager> theCollection;
  ComponentPtrType<group::GroupManager> theGroup;
  ComponentPtrType<pipe::PipeManager> theCB;
  ComponentPtrType<objgroup::ObjGroupManager> theObjGroup;
  ComponentPtrType<util::memory::MemoryUsage> theMemUsage;
  ComponentPtrType<rdma::Manager> theHandleRDMA;
  ComponentPtrType<vrt::collection::balance::ProcStats> theProcStats;
  ComponentPtrType<vrt::collection::balance::StatsRestartReader> theStatsReader;
  ComponentPtrType<vrt::collection::balance::LBManager> theLBManager;

  // Node-level worker-based components for vt (these are optional)
  ComponentPtrType<worker::WorkerGroupType> theWorkerGrp;

  // Optional components
  #if vt_check_enabled(trace_enabled)
    ComponentPtrType<trace::Trace> theTrace = nullptr;
  #endif

  static bool volatile sig_user_1_;

protected:
  bool finalize_on_term_ = false;
  bool initialized_ = false, finalized_ = false, aborted_ = false;
  bool runtime_active_ = false;
  bool is_interop_ = false;
  WorkerCountType num_workers_ = no_workers;
  MPI_Comm communicator_ = MPI_COMM_NULL;
  int user_argc_ = 0;
  std::unique_ptr<char*[]> user_argv_ = nullptr;
  std::unique_ptr<component::ComponentPack> p_;
};

}} /* end namespace vt::runtime */

#endif /*INCLUDED_RUNTIME_H*/
