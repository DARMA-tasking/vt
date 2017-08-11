
#if ! defined __RUNTIME_TRANSPORT_COLLECTIVE__
#define __RUNTIME_TRANSPORT_COLLECTIVE__

#include "common.h"
#include "context.h"
#include "registry.h"
#include <mpi.h>

namespace runtime {

struct CollectiveOps {
  static void
  initialize_context(int argc, char** argv) {
    int num_nodes = 0, this_node = 0;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_nodes);
    MPI_Comm_rank(MPI_COMM_WORLD, &this_node);
    the_context = std::make_unique<Context>(this_node, num_nodes);
  }

  static handler_t
  register_handler(active_function_t fn) {
    auto han = the_registry->register_active_handler(
      static_cast<active_function_t>(fn)
    );
    MPI_Barrier(MPI_COMM_WORLD);
    return han;
  }

  static void
  finalize_context() {
    MPI_Barrier(MPI_COMM_WORLD);
    the_context = nullptr;
    MPI_Finalize();
  }

  static void
  initialize_runtime();

  static void
  finalize_runtime();
};

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_COLLECTIVE__*/
