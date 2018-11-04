
#if !defined INCLUDED_RUNTIME_H
#define INCLUDED_RUNTIME_H

#include "vt/config.h"
#include "vt/runtime/runtime_common.h"
#include "vt/context/context.h"
#include "vt/registry/registry.h"
#include "vt/messaging/active.h"
#include "vt/event/event.h"
#include "vt/termination/term_headers.h"
#include "vt/collective/collective_alg.h"
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
#include "vt/group/group_headers.h"
#include "vt/pipe/pipe_headers.h"
#include "vt/runtime/runtime_get.h"

#include <memory>
#include <functional>
#include <string>

#include <mpi.h>

namespace vt { namespace runtime {

struct Runtime {
  template <typename ComponentT>
  using ComponentPtrType = std::unique_ptr<ComponentT>;

  Runtime(
    int argc = 0, char** argv = nullptr,
    WorkerCountType in_num_workers = no_workers, bool const interop_mode = false,
    MPI_Comm* in_comm = nullptr,
    RuntimeInstType const in_instance = RuntimeInstType::DefaultInstance
  );

  Runtime(Runtime const&) = delete;
  Runtime(Runtime&&) = delete;
  Runtime& operator=(Runtime const&) = delete;

  virtual ~Runtime();

  bool isTerminated() const { return not runtime_active_; }
  bool isFinializeble() const { return initialized_ and not finalized_; }
  bool isInitialized() const { return initialized_; }
  bool isFinalized() const { return finalized_; }
  bool hasSchedRun() const { return theSched ? theSched->hasSchedRun() : false; }

  void reset();
  bool initialize(bool const force_now = false);
  bool finalize(bool const force_now = false);
  void runScheduler();
  void abort(std::string const abort_str, ErrorCodeType const code);
  void output(
    std::string const abort_str, ErrorCodeType const code, bool error = false,
    bool decorate = true
  );

  RuntimeInstType getInstanceID() const { return instance_; }

private:
  RuntimeInstType const instance_;

protected:
  bool tryInitialize();
  bool tryFinalize();

  void initializeContext(int argc, char** argv, MPI_Comm* comm);
  void initializeTrace();
  void initializeComponents();
  void initializeOptionalComponents();
  void initializeWorkers(WorkerCountType const num_workers);

  void finalizeContext();
  void finalizeTrace();
  void finalizeComponents();
  void finalizeOptionalComponents();

  void sync();
  void setup();
  void terminationHandler();
  void printStartupBanner();
  void printShutdownBanner(term::TermCounterType const& num_units);

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

  // Node-level worker-based components for vt (these are optional)
  ComponentPtrType<worker::WorkerGroupType> theWorkerGrp;

  // Optional components
  #if backend_check_enabled(trace_enabled)
    ComponentPtrType<trace::Trace> theTrace = nullptr;
  #endif

protected:
  bool finalize_on_term_ = false;
  bool initialized_ = false, finalized_ = false, aborted_ = false;
  bool runtime_active_ = false;
  bool is_interop_ = false;
  WorkerCountType num_workers_ = no_workers;
  MPI_Comm* communicator_ = nullptr;
  int user_argc_ = 0;
  char** user_argv_ = nullptr;
};

}} /* end namespace vt::runtime */

#endif /*INCLUDED_RUNTIME_H*/
