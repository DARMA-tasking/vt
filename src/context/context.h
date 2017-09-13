
#if ! defined __RUNTIME_TRANSPORT_CONTEXT__
#define __RUNTIME_TRANSPORT_CONTEXT__

#include "common.h"

#include <memory>

namespace vt {

struct Context {
  Context(
    NodeType const& in_this_node, NodeType const& in_num_nodes
  ) : this_node(in_this_node), num_nodes(in_num_nodes)
  { }

  inline NodeType getNode() const { return this_node; }
  inline NodeType getNumNodes() const { return num_nodes; }

private:
  NodeType this_node = 0;
  NodeType num_nodes = 0;
};

extern std::unique_ptr<Context> theContext;

} //end namespace vt

#endif /*__RUNTIME_TRANSPORT_CONTEXT__*/
