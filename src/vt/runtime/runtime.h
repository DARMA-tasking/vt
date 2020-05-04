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

#if backend_check_enabled(trace_enabled)
#include "vt/trace/trace.h"
#endif

#include <memory>
#include <functional>
#include <string>

#include <mpi.h>

namespace vt { namespace runtime {

struct Runtime {
  template <typename ComponentT>
  using ComponentPtrType = ComponentT*;
  using ArgType = vt::arguments::ArgConfig;

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

  bool isLive() const { return p_ and p_->isLive(); }
  int progress() { if (p_) return p_->progress(); else return 0; }
  bool isTerminated() const { return not runtime_active_; }
  bool isFinializeble() const { return initialized_ and not finalized_; }
  bool isInitialized() const { return initialized_; }
  bool isFinalized() const { return finalized_; }
  bool hasSchedRun() const;

  void reset();
  bool initialize(bool const force_now = false);
  bool finalize(bool const force_now = false);
  void runScheduler();
  void abort(std::string const abort_str, ErrorCodeType const code);
  void output(
    std::string const abort_str, ErrorCodeType const code, bool error = false,
    bool decorate = true, bool formatted = true
  );

  RuntimeInstType getInstanceID() const { return instance_; }

  void systemSync() { sync(); }

public:
  void checkForArgumentErrors();

private:
  RuntimeInstType const instance_;

  static bool nodeStackWrite();
  static void writeToFile(std::string const& str);
  static std::string prog_name_;

protected:
  bool tryInitialize();
  bool tryFinalize();

  void initializeErrorHandlers();
  void initializeComponents();
  void initializeOptionalComponents();
  void initializeWorkers(WorkerCountType const num_workers);
  bool needStatsRestartReader();

  void finalizeMPI();

  void sync();
  void setup();
  void terminationHandler();
  void printStartupBanner();
  void printShutdownBanner(
    term::TermCounterType const& num_units, std::size_t const coll_epochs
  );

  void pauseForDebugger();
  void setupSignalHandler();
  void setupSignalHandlerINT();
  void setupTerminateHandler();
  static void sigHandler(int sig);
  static void sigHandlerUsr1(int sig);
  static void sigHandlerINT(int sig);
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
  #if backend_check_enabled(trace_enabled)
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
  char** user_argv_ = nullptr;
  std::unique_ptr<component::ComponentPack> p_;
};

}} /* end namespace vt::runtime */

#endif /*INCLUDED_RUNTIME_H*/
