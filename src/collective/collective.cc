
#include "collective.h"
#include "termination.h"
#include "barrier.h"
#include "trace.h"
#include "transport.h"

#include <mpi.h>

namespace vt {

bool vtIsWorking = true;

/*static*/ void CollectiveOps::initialize(int argc, char** argv) {
  initializeContext(argc, argv);
  initializeRuntime();
}

/*static*/ void CollectiveOps::finalize() {
  MPI_Barrier(MPI_COMM_WORLD);

  finalizeSingletons();
  finalizeContext();
  finalizeRuntime();
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
}

/*static*/ HandlerType CollectiveOps::registerHandler(ActiveFunctionType fn) {
  return theRegistry->registerActiveHandler(fn);
}

/*static*/ void CollectiveOps::initializeRuntime() {
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
  // set trace to nullptr to write out to disk
  backend_enable_if(
    trace_enabled, theTrace = nullptr;
  );

  MPI_Barrier(MPI_COMM_WORLD);
}

/*static*/ void CollectiveOps::finalizeContext() {
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
  theEvent = nullptr;
  thePool = nullptr;
}

} //end namespace vt
