
#if ! defined __RUNTIME_TRANSPORT_COLLECTIVE__
#define __RUNTIME_TRANSPORT_COLLECTIVE__

#include "configs/types/types_common.h"
#include "context.h"
#include "registry.h"

#include <mpi.h>

namespace vt {

struct CollectiveOps {
  static void initializeContext(int argc, char** argv) {
    int num_nodes = 0, this_node = 0;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &num_nodes);
    MPI_Comm_rank(MPI_COMM_WORLD, &this_node);
    theContext = std::make_unique<Context>(this_node, num_nodes);
    MPI_Barrier(MPI_COMM_WORLD);
  }

  static HandlerType registerHandler(ActiveFunctionType fn) {
    return theRegistry->registerActiveHandler(fn);
  }

  static void finalizeContext();
  static void initializeRuntime();
  static void finalizeRuntime();
};

} //end namespace vt

#endif /*__RUNTIME_TRANSPORT_COLLECTIVE__*/
