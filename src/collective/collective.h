
#if ! defined __RUNTIME_TRANSPORT_COLLECTIVE__
#define __RUNTIME_TRANSPORT_COLLECTIVE__

#include "common.h"
#include "context.h"
#include "registry.h"

#include <mpi.h>

namespace vt {

struct CollectiveOps {
  static void
  initialize_context(int argc, char** argv) {
    int num_nodes = 0, this_node = 0;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_nodes);
    MPI_Comm_rank(MPI_COMM_WORLD, &this_node);
    the_context = std::make_unique<Context>(this_node, num_nodes);
    MPI_Barrier(MPI_COMM_WORLD);
  }

  static HandlerType
  register_handler(ActiveFunctionType fn) {
    return the_registry->register_active_handler(fn);
  }

  static void
  finalize_context();

  static void
  initialize_runtime();

  static void
  finalize_runtime();
};

} //end namespace vt

#endif /*__RUNTIME_TRANSPORT_COLLECTIVE__*/
