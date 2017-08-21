
#include "collective.h"
#include "transport.h"

namespace runtime {

/*static*/ void
CollectiveOps::initialize_runtime() {
  term::TerminationDetector::register_termination_handlers();
  term::TerminationDetector::register_default_termination_action();
  AsyncEvent::register_event_handlers();
  barrier::Barrier::register_barrier_handlers();
  rdma::RDMAManager::register_all_rdma_handlers();

  // wait for all nodes to start up to initialize the runtime
  //the_barrier->barrier();
}

/*static*/ void
CollectiveOps::finalize_runtime() {
  // wait for all nodes to wind down to finalize the runtime
  the_barrier->system_meta_barrier_cont([]{
    ///MPI_Finalize();

    // exit the program
    exit(0);
  });
}

/*static*/ void
CollectiveOps::finalize_context() {
  // int buf = 0, buf2 = 0, flag = 0;
  // MPI_Request req;
  // MPI_Status stat;
  // auto const& node = the_context->get_node();
  // fflush(stdout);
  // printf("%d: MPI_Iallreduce\n", node);
  // MPI_Iallreduce(&buf, &buf2, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD, &req);
  // do {
  //   MPI_Test(&req, &flag, &stat);
  //   the_msg->scheduler();
  // } while (not flag);
  // MPI_Barrier(MPI_COMM_WORLD);
  // the_context = nullptr;
  //
}

} //end namespace runtime
