
#include "config.h"
#include "runtime.h"

#include "context/context.h"
#include "registry/registry.h"
#include "messaging/active.h"
#include "event/event.h"
#include "termination/termination.h"
#include "barrier/barrier.h"
#include "pool/pool.h"
#include "rdma/rdma.h"
#include "parameterization/parameterization.h"
#include "sequence/sequencer_headers.h"
#include "trace/trace.h"
#include "scheduler/scheduler.h"
#include "topos/location/location.h"
#include "vrt/context/context_vrtmanager.h"
#include "worker/worker_headers.h"

#include <memory>

namespace vt { namespace runtime {

Runtime::Runtime(
  int argc, char** argv, WorkerCountType in_num_workers,
  bool const interop_mode, MPI_Comm* in_comm, RuntimeInstType const in_instance
)  : instance_(in_instance), runtime_active_(false), is_interop_(interop_mode),
     num_workers_(in_num_workers), communicator_(in_comm), user_argc_(argc),
     user_argv_(argv)
{ }

/*virtual*/ Runtime::~Runtime() {
  while (runtime_active_) {
    runScheduler();
  }
  finalize();
}

bool Runtime::tryInitialize() {
  bool const init_now = !initialized_ && !finalized_;

  debug_print(
    runtime, unknown,
    "Runtime: tryInitialize: initialized_=%s, finalized_=%s, init_now=%s\n",
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
    "Runtime: tryFinalize: initialized_=%s, finalized_=%s, rt_live=%s, sched=%s, "
    "finalize_now=%s\n",
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

bool Runtime::initialize(bool const force_now) {
  if (force_now) {
    initializeContext(user_argc_, user_argv_, communicator_);
    initializeComponents();
    initializeOptionalComponents();
    sync();
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
    sync();
    finalizeOptionalComponents();
    finalizeComponents();
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

void Runtime::terminationHandler() {
  debug_print(
    runtime, node,
    "Runtime: executing registered termination handler\n",
  );

  runtime_active_ = false;

  if (finalize_on_term_) {
    finalize();
  }
}

void Runtime::setup() {
  debug_print(runtime, node, "begin: setup\n");

  runtime_active_ = true;

  auto handler = std::bind(&Runtime::terminationHandler, this);
  term::TerminationDetector::registerDefaultTerminationAction(handler);

  MPI_Barrier(theContext->getComm());

  // wait for all nodes to start up to initialize the runtime
  theBarrier->barrierThen([this]{
    MPI_Barrier(theContext->getComm());
  });

  backend_enable_if(
    trace_enabled, {
      std::string name = argc == 0 ? "prog" : argv[0];
      auto const& node = theContext->getNode();
      theTrace->setupNames(name, name + "." + std::to_string(node) + ".log.gz");
    }
  );

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

  theRegistry = std::make_unique<Registry>();
  theMsg = std::make_unique<ActiveMessenger>();
  theEvent = std::make_unique<event::AsyncEvent>();
  theTerm = std::make_unique<term::TerminationDetector>();
  theBarrier = std::make_unique<barrier::Barrier>();
  thePool = std::make_unique<pool::Pool>();
  theRDMA = std::make_unique<rdma::RDMAManager>();
  theParam = std::make_unique<param::Param>();
  theSeq = std::make_unique<seq::Sequencer>();
  theVirtualSeq = std::make_unique<seq::SequencerVirtual>();
  theSched = std::make_unique<sched::Scheduler>();
  theLocMan = std::make_unique<location::LocationManager>();
  theVirtualManager = std::make_unique<vrt::VirtualContextManager>();

  debug_print(runtime, node, "end: initializeComponents\n");
}

void Runtime::initializeOptionalComponents() {
  debug_print(runtime, node, "begin: initializeOptionalComponents\n");

  backend_enable_if(
    trace_enabled,
    theTrace = std::make_unique<trace::Trace>();
  );

  initializeWorkers(num_workers_);

  debug_print(runtime, node, "end: initializeOptionalComponents\n");
}

void Runtime::initializeWorkers(WorkerCountType const num_workers) {
  debug_print(
    runtime, node, "begin: initializeWorkers: workers=%d\n", num_workers
  );

  bool const has_workers = num_workers != no_workers;

  if (has_workers) {
    theContext->setNumWorkers(num_workers);
    theWorkerGrp = std::make_unique<worker::WorkerGroupType>();
  }

  debug_print(runtime, node, "end: initializeWorkers\n");
}

void Runtime::finalizeComponents() {
  debug_print(runtime, node, "begin: finalizeComponents\n");

  theParam = nullptr;
  theSeq = nullptr;
  theLocMan = nullptr;
  theTerm = nullptr;
  theBarrier = nullptr;
  theRDMA = nullptr;
  theSched = nullptr;
  theMsg = nullptr;
  theRegistry = nullptr;
  theEvent->cleanup(); theEvent = nullptr;
  thePool = nullptr;

  debug_print(runtime, node, "end: finalizeComponents\n");
}

void Runtime::finalizeOptionalComponents() {
  debug_print(runtime, node, "begin: finalizeOptionalComponents\n");

  theWorkerGrp = nullptr;

  debug_print(runtime, node, "end: finalizeOptionalComponents\n");
}

}} //end namespace vt::runtime
