
#include "config.h"
#include "collective/tree/tree.h"
#include "context/context.h"

namespace vt { namespace collective { namespace tree {

Tree::Tree(DefaultTreeConstructTag) {
  setupTree();
}

Tree::Tree(NodeListType const& in_children) {
  is_root_ = true;
  children_ = in_children;
}

NodeType Tree::getParent() const {
  return parent_;
}

NodeType Tree::getNumChildren() const {
  return children_.size();
}

bool Tree::isRoot() const {
  return is_root_;
}

Tree::NodeListType const& Tree::getChildren() const {
  return children_;
}

void Tree::foreachChild(OperationType op) const {
  if (children_.size() > 0) {
    for (auto&& elm : children_) {
      op(elm);
    }
  }
}

void Tree::setupTree() {
  if (not set_up_tree_) {
    auto const& this_node_ = theContext()->getNode();
    auto const& num_nodes_ = theContext()->getNumNodes();

    auto const& c1_ = this_node_ * 2 + 1;
    auto const& c2_ = this_node_ * 2 + 2;

    if (c1_ < num_nodes_) {
      children_.push_back(c1_);
    }
    if (c2_ < num_nodes_) {
      children_.push_back(c2_);
    }

    is_root_ = this_node_ == 0;

    if (not is_root_) {
      parent_ = (this_node_ - 1) / 2;
    }

    set_up_tree_ = true;
  }
}

}}} //end namespace vt::collective::tree
