
#include "collective.h"
#include "termination.h"
#include "barrier.h"
#include "trace.h"
#include "transport.h"
#include "vrt/vrt_contextmanager.h"


#include <mpi.h>

namespace vt {

bool vtIsWorking = true;

/*static*/ void CollectiveOps::initialize(int argc, char** argv) {
  initializeContext(argc, argv);
  initializeSingletons();

  // wait for all singletons to be initialized
  MPI_Barrier(MPI_COMM_WORLD);

  // start up runtime
  initializeRuntime();
}

/*static*/ void CollectiveOps::finalize() {
  MPI_Barrier(MPI_COMM_WORLD);

  finalizeRuntime();
  finalizeSingletons();
  finalizeContext();
}

/*static*/ void CollectiveOps::setInactiveState() {
  vtIsWorking = false;
}

/*static*/ void CollectiveOps::initializeContext(int argc, char** argv) {
  int num_nodes = 0, this_node = 0;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &num_nodes);
  MPI_Comm_rank(MPI_COMM_WORLD, &this_node);
  theContext = std::make_unique<Context>(this_node, num_nodes);
  MPI_Barrier(MPI_COMM_WORLD);

  debug_print(gen, node, "initializeContext\n");
}

/*static*/ HandlerType CollectiveOps::registerHandler(ActiveFunctionType fn) {
  return theRegistry->registerActiveHandler(fn);
}

/*static*/ void CollectiveOps::initializeRuntime() {
  debug_print(gen, node, "initializeRuntime\n");

  vtIsWorking = true;

  term::TerminationDetector::registerDefaultTerminationAction();

  MPI_Barrier(MPI_COMM_WORLD);

  // wait for all nodes to start up to initialize the runtime
  theBarrier->barrierThen([]{
    MPI_Barrier(MPI_COMM_WORLD);
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

/*static*/ void CollectiveOps::finalizeContext() {
  MPI_Barrier(MPI_COMM_WORLD);
  MPI_Finalize();
  // int buf = 0, buf2 = 0, flag = 0;
  // MPI_Request req;
  // MPI_Status stat;
  // auto const& node = theContext->getNode();
  // fflush(stdout);
  // printf("%d: MPI_Iallreduce\n", node);
  // MPI_Iallreduce(&buf, &buf2, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD, &req);
  // do {
  //   MPI_Test(&req, &flag, &stat);
  //   theMsg->scheduler();
  // } while (not flag);
  // MPI_Barrier(MPI_COMM_WORLD);
  // theContext = nullptr;
  //
}

/*static*/ void CollectiveOps::initializeSingletons() {
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
  theSched = std::make_unique<sched::Scheduler>();
  theLocMan = std::make_unique<location::LocationManager>();
  theVrtCManager = std::make_unique<vrt::VrtContextManager>();

  backend_enable_if(
    trace_enabled, theTrace = std::make_unique<trace::Trace>();
  );

  debug_print(gen, node, "initializeSingletons finished\n");
}

/*static*/ void CollectiveOps::finalizeSingletons() {
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

//  theVrtCManager = nullptr;

  // set trace to nullptr to write out to disk
  backend_enable_if(
    trace_enabled, theTrace = nullptr;
  );
}

} //end namespace vt
