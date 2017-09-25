
#if !defined INCLUDED_CONTEXT
#define INCLUDED_CONTEXT

#include <memory>

#include "config.h"

namespace vt {

struct Context {
  Context(NodeType const& in_this_node, NodeType const& in_num_nodes)
      : thisNode_(in_this_node), numNodes_(in_num_nodes) {}

  inline NodeType getNode() const { return thisNode_; }
  inline NodeType getNumNodes() const { return numNodes_; }

 private:
  NodeType thisNode_ = 0;
  NodeType numNodes_ = 0;
};

extern std::unique_ptr<Context> theContext;

}  // end namespace vt

#endif /*INCLUDED_CONTEXT*/
