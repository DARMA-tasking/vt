
#include "collective.h"
#include "termination/termination.h"
#include "barrier/barrier.h"
#include "trace/trace.h"
#include "transport.h"
#include "vrt/context/context_vrtmanager.h"
#include "sequence/sequencer_headers.h"
#include "worker/worker_headers.h"

#include <mpi.h>

namespace vt {

bool vtIsWorking = true;

/*static*/ void finalizeInterop() {

}

/*static*/ void CollectiveOps::initialize(
  int argc, char** argv, WorkerCountType const num_workers,
  bool is_interop, MPI_Comm* comm
) {
  initializeContext(argc, argv, is_interop, comm);
  initializeComponents();
  initializeWorkers(num_workers);

  // wait for all singletons to be initialized
  MPI_Barrier(theContext->getComm());

  // start up runtime
  initializeRuntime();
}

/*static*/ void CollectiveOps::finalize() {
  MPI_Barrier(theContext->getComm());

  finalizeRuntime();
  finalizeWorkers();
  finalizeComponents();
  finalizeContext();
}

/*static*/ void CollectiveOps::setInactiveState() {
  vtIsWorking = false;
}

/*static*/ void CollectiveOps::initializeContext(
  int argc, char** argv, bool is_interop, MPI_Comm* comm
) {
  theContext = std::make_unique<ctx::Context>(argc, argv, is_interop, comm);

  debug_print(gen, node, "initializeContext\n");
}

/*static*/ HandlerType CollectiveOps::registerHandler(ActiveClosureFnType fn) {
  return theRegistry->registerActiveHandler(fn);
}

/*static*/ void CollectiveOps::initializeRuntime() {
  debug_print(gen, node, "initializeRuntime\n");

  vtIsWorking = true;

  term::TerminationDetector::registerDefaultTerminationAction();

  MPI_Barrier(theContext->getComm());

  // wait for all nodes to start up to initialize the runtime
  theBarrier->barrierThen([]{
    MPI_Barrier(theContext->getComm());
  });

  backend_enable_if(
    trace_enabled, {
      std::string name = "prog";
      auto const& node = theContext->getNode();
      theTrace->setupNames(name, name + "." + std::to_string(node) + ".log.gz");
    }
  );
}

/*static*/ void
CollectiveOps::finalizeRuntime() {
}

/*static*/ void CollectiveOps::finalizeContext(bool is_interop) {
  MPI_Barrier(theContext->getComm());

  if (not is_interop) {
    MPI_Finalize();
  }
}

/*static*/ void CollectiveOps::initializeComponents() {
  debug_print(gen, node, "initializeSingletons\n");

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
  theWorkerGrp = std::make_unique<worker::WorkerGroup>();

  backend_enable_if(
    trace_enabled, theTrace = std::make_unique<trace::Trace>();
  );

  debug_print(gen, node, "initializeSingletons finished\n");
}

/*static*/ void CollectiveOps::initializeWorkers(
  WorkerCountType const num_workers
) {
  debug_print(gen, node, "initializeWorkers\n");

  if (num_workers != no_workers) {
    theContext->setNumWorkers(num_workers);
    theWorkerGrp = std::make_unique<worker::WorkerGroup>();
  }
}

/*static*/ void CollectiveOps::finalizeWorkers() {
  debug_print(gen, node, "finalizeWorkers\n");
  theWorkerGrp = nullptr;
}

/*static*/ void CollectiveOps::finalizeComponents() {
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

//  theVirtualManager = nullptr;

  // set trace to nullptr to write out to disk
  backend_enable_if(
    trace_enabled, theTrace = nullptr;
  );
}

} //end namespace vt
