
#if ! defined __RUNTIME_TRANSPORT_CONTEXT__
#define __RUNTIME_TRANSPORT_CONTEXT__

#include "common.h"

namespace runtime {

struct Context {
  Context(
    node_t const& in_this_node, node_t const& in_num_nodes
  ) : this_node(in_this_node), num_nodes(in_num_nodes)
  { }

  node_t get_node() const { return this_node; }
  node_t get_num_nodes() const { return num_nodes; }

private:
  node_t this_node = 0;
  node_t num_nodes = 0;
};

extern std::unique_ptr<Context> the_context;

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_CONTEXT__*/
