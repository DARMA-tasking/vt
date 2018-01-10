
#if !defined INCLUDED_RUNTIME_H
#define INCLUDED_RUNTIME_H

#include "config.h"
#include "runtime/runtime_common.h"
#include "context/context.h"
#include "registry/registry.h"
#include "messaging/active.h"
#include "event/event.h"
#include "termination/term_headers.h"
#include "barrier/barrier.h"
#include "pool/pool.h"
#include "rdma/rdma_headers.h"
#include "parameterization/parameterization.h"
#include "sequence/sequencer_headers.h"
#include "trace/trace.h"
#include "scheduler/scheduler.h"
#include "topos/location/location.h"
#include "vrt/context/context_vrtmanager.h"
#include "vrt/collection/collection_headers.h"
#include "worker/worker_headers.h"
#include "runtime_get.h"

#include <memory>
#include <functional>
#include <string>

#include <mpi.h>

namespace vt { namespace runtime {

struct Runtime {
  template <typename ComponentT>
  using ComponentPtr = std::unique_ptr<ComponentT>;

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

  bool initialize(bool const force_now = false);
  bool finalize(bool const force_now = false);
  void runScheduler();
  void abort(std::string const abort_str, ErrorCodeType const code);

  RuntimeInstType getInstanceID() const { return instance_; }

private:
  RuntimeInstType const instance_;

protected:
  bool tryInitialize();
  bool tryFinalize();

  void initializeContext(int argc, char** argv, MPI_Comm* comm);
  void initializeComponents();
  void initializeOptionalComponents();
  void initializeWorkers(WorkerCountType const num_workers);

  void finalizeContext();
  void finalizeComponents();
  void finalizeOptionalComponents();

  void sync();
  void setup();
  void terminationHandler();
  void printStartupBanner();
  void printShutdownBanner();

public:
  ComponentPtr<Registry> theRegistry;
  ComponentPtr<ActiveMessenger> theMsg;
  ComponentPtr<ctx::Context> theContext;
  ComponentPtr<event::AsyncEvent> theEvent;
  ComponentPtr<term::TerminationDetector> theTerm;
  ComponentPtr<barrier::Barrier> theBarrier;
  ComponentPtr<pool::Pool> thePool;
  ComponentPtr<rdma::RDMAManager> theRDMA;
  ComponentPtr<param::Param> theParam;
  ComponentPtr<seq::Sequencer> theSeq;
  ComponentPtr<seq::SequencerVirtual> theVirtualSeq;
  ComponentPtr<sched::Scheduler> theSched;
  ComponentPtr<location::LocationManager> theLocMan;
  ComponentPtr<vrt::VirtualContextManager> theVirtualManager;
  ComponentPtr<vrt::collection::CollectionManager> theCollection;

  // Node-level worker-based components for vt (these are optional)
  ComponentPtr<worker::WorkerGroupType> theWorkerGrp;

  // Optional components
  #if backend_check_enabled(trace_enabled)
    ComponentPtr<trace::Trace> theTrace = nullptr;
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
