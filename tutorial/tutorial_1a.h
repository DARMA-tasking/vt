
#include "transport.h"

namespace vt { namespace tutorial {

static inline void context() {
  /*
   *  ::vt::theContext() can be used to obtain the node and number of nodes:
   *  these correlate exactly to the MPI ranks and MPI size.
   *
   *  By default if you initialize VT without passing an MPI communication, VT
   *  will initialize MPI and use an internal communicator. If you want VT to
   *  interoperate, you can pass a communicator to the initialize invocation.
   *
   */

  // Equivalent to: MPI_Comm_rank(...)
  NodeType const this_node = ::vt::theContext()->getNode();

  // Equivalent to: MPI_Comm_size(...)
  NodeType const num_nodes = ::vt::theContext()->getNumNodes();

  // The header-only library fmt is used for printing throughout VT. You can use
  // it because the headers are included by default
  ::fmt::print("this_node={}, num_ndoes={}\n", this_node, num_nodes);
}

}} /* end namespace vt::tutorial */
