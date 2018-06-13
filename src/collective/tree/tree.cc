
#include "config.h"
#include "collective/tree/tree.h"
#include "context/context.h"

#include <cmath>
#include <cstdlib>

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

Tree::NodeListType&& Tree::getChildren(NodeType node) const {
  auto const& num_nodes = theContext()->getNumNodes();
  auto const& c1_ = node * 2 + 1;
  auto const& c2_ = node * 2 + 2;
  NodeListType children;
  if (c1_ < num_nodes) {
    children.push_back(c1_);
  }
  if (c2_ < num_nodes) {
    children.push_back(c2_);
  }
  return std::move(children);
}

std::size_t Tree::getNumTotalChildren(NodeType child) const {
  std::size_t total = 0;
  auto children = getChildren(child);
  for (auto&& elm : children) {
    total += getNumTotalChildren(elm);
  }
  return total;
}

std::size_t Tree::getNumTotalChildren() const {
  auto total_size = 0;
  foreachChild([this,&total_size](NodeType child){
    total_size += getNumTotalChildren(child);
  });
  return total_size;
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

void Tree::foreachChild(NumLevelsType level, OperationType op) const {
  int32_t const start = std::pow(2.0f, level) - 1;
  int32_t const end = std::pow(2.0f, level + 1) - 1;
  for (auto i = start; i < end; i++) {
    op(i);
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

Tree::NumLevelsType Tree::numLevels() const {
  auto const& num_nodes = theContext()->getNumNodes();
  auto const& levels = std::log2(num_nodes);
  return levels;
}

}}} //end namespace vt::collective::tree
