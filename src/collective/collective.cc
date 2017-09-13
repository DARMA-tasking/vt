
#include "collective.h"
#include "termination.h"
#include "barrier.h"
#include "trace.h"

namespace vt {

/*static*/ void CollectiveOps::initializeRuntime() {
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
      theTrace->setup_names(name, name + "." + std::to_string(node) + ".log.gz");
    }
  );
}

/*static*/ void
CollectiveOps::finalizeRuntime() {
  // wait for all nodes to wind down to finalize the runtime
  theBarrier->systemMetaBarrierCont([]{
    ///MPI_Finalize();

    // set trace to nullptr to write out to disk
    backend_enable_if(
      trace_enabled, theTrace = nullptr;
    );

    MPI_Barrier(MPI_COMM_WORLD);

    // exit the program
    exit(0);
  });
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

} //end namespace vt
