
#include "context/context.h"

#include <mpi.h>

// This cannot use the normal debug_print macros because they rely on context
// being live to print contextual information
#define DEBUG_VT_CONTEXT 0

namespace vt { namespace ctx {

Context::Context(int argc, char** argv, bool const is_interop, MPI_Comm* comm) {
  #if DEBUG_VT_CONTEXT
    printf(
      "Context::Context is_interop=%s, comm=%p\n", print_bool(is_interop), comm
    );
  #endif

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

Context::Context(bool const interop, MPI_Comm* comm)
  : Context(0, nullptr, interop, comm)
{ }

}}  // end namespace vt::ctx

#undef DEBUG_VT_CONTEXT
