
#include "context/context.h"

#include <mpi.h>

namespace vt { namespace ctx {

Context::Context(int argc, char** argv, bool const is_interop, MPI_Comm* comm) {
  printf(
    "Context::Context is_interop=%s, comm=%p\n", print_bool(is_interop), comm
  );

  if (not is_interop) {
    MPI_Init(&argc, &argv);
  }

  if (comm != nullptr) {
    communicator_ = *comm;
  } else {
    communicator_ = MPI_COMM_WORLD;
  }

  MPI_Barrier(communicator_);

  if (is_interop) {
    MPI_Comm_split(communicator_, 0, 0, &communicator_);
  }

  MPI_Barrier(communicator_);

  is_comm_world_ = communicator_ == MPI_COMM_WORLD;

  int numNodesLocal = uninitialized_destination;
  int thisNodeLocal = uninitialized_destination;

  MPI_Comm_size(communicator_, &numNodesLocal);
  MPI_Comm_rank(communicator_, &thisNodeLocal);

  numNodes_ = static_cast<NodeType>(numNodesLocal);
  thisNode_ = static_cast<NodeType>(thisNodeLocal);
}

}}  // end namespace vt::ctx
