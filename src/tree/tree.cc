
#include "tree.h"
#include "context.h"

namespace vt {

void
Tree::setupTree() {
  if (not set_up_tree_) {
    my_node_ = theContext->getNode();
    num_nodes_ = theContext->getNumNodes();

    c1_ = my_node_*2+1;
    c2_ = my_node_*2+2;
    has_c1_ = c1_ < num_nodes_;
    has_c2_ = c2_ < num_nodes_;
    num_children_ = has_c1_ + has_c2_;

    is_root_ = my_node_ == 0;

    if (not is_root_) {
      parent_ = (my_node_ - 1) / 2;
    }

    set_up_tree_ = true;
  }
}

} //end namespace vt

