/*
//@HEADER
// ************************************************************************
//
//                          runtime.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_RUNTIME_H
#define INCLUDED_RUNTIME_H

#include "vt/config.h"
#include "vt/runtime/runtime_common.h"
#include "vt/runtime/runtime_component_fwd.h"
#include "vt/trace/trace.h"
#include "vt/worker/worker_headers.h"
#include "vt/configs/arguments/args.h"

#include <memory>
#include <functional>
#include <string>

#include <mpi.h>

namespace vt { namespace runtime {

struct Runtime {
  template <typename ComponentT>
  using ComponentPtrType = std::unique_ptr<ComponentT>;

  Runtime(
    int& argc, char**& argv,
    WorkerCountType in_num_workers = no_workers, bool const interop_mode = false,
    MPI_Comm* in_comm = nullptr,
    RuntimeInstType const in_instance = RuntimeInstType::DefaultInstance
  );

  Runtime(
    bool const interop_mode = false,
    RuntimeInstType const in_instance = RuntimeInstType::DefaultInstance
  );

  Runtime(Runtime const&) = delete;
  Runtime(Runtime&&) = delete;
  Runtime& operator=(Runtime const&) = delete;

  virtual ~Runtime();

  void setArgConfigs(int &argc, char**& argv) {
	  const vt::arguments::Configs myDefault;
	  this->setArgConfigs(argc, argv, myDefault);
  }

  void setArgConfigs(
    int &argc, char**& argv, 
	const vt::arguments::Configs &ref
  );

  void setMPIComm(MPI_Comm* in_comm) { communicator_ = in_comm; }
  void setNumWorkers(WorkerCountType in_num_workers) { num_workers_ = in_num_workers; }

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

private:
  RuntimeInstType const instance_;

  static bool nodeStackWrite();
  static void writeToFile(std::string const& str);
  static std::string prog_name_;

protected:
  bool tryInitialize();
  bool tryFinalize();

  void initializeContext(int argc, char** argv, MPI_Comm* comm);
  void initializeTrace();
  void initializeErrorHandlers();
  void initializeComponents();
  void initializeOptionalComponents();
  void initializeWorkers(WorkerCountType const num_workers);

  void finalizeContext();
  void finalizeTrace();
  void finalizeComponents();
  void finalizeOptionalComponents();

  void parseAndSetup(int& argc, char**& argv, const vt::arguments::Configs &ref);

  void sync();
  void setup();
  void terminationHandler();
  void printStartupBanner();
  void printShutdownBanner(term::TermCounterType const& num_units);

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
  MPI_Comm* communicator_ = nullptr;
  int user_argc_ = 0;
  char** user_argv_ = nullptr;
  bool parsed_arg = false;
};

}} /* end namespace vt::runtime */

#endif /*INCLUDED_RUNTIME_H*/
