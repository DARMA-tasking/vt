
#if !defined INCLUDED_COLLECTIVE_TREE_TREE_H
#define INCLUDED_COLLECTIVE_TREE_TREE_H

#include "config.h"

#include <vector>
#include <functional>
#include <cstdlib>

namespace vt { namespace collective { namespace tree {

#pragma sst keep
static struct DefaultTreeConstructTag { } tree_cons_tag_t { };

struct Tree {
  using NodeListType = std::vector<NodeType>;
  using OperationType = std::function<void(NodeType)>;
  using NumLevelsType = int32_t;

  explicit Tree(DefaultTreeConstructTag);
  explicit Tree(NodeListType const& in_children);

  void setupTree();
  NodeType getParent() const;
  NodeType getNumChildren() const;
  bool isRoot() const;
  NodeListType const& getChildren() const;
  NodeListType getChildren(NodeType node) const;
  void foreachChild(OperationType op) const;
  NumLevelsType numLevels() const;
  void foreachChild(NumLevelsType level, OperationType op) const;
  std::size_t getNumTotalChildren(NodeType child) const;
  std::size_t getNumTotalChildren() const;

private:
  bool set_up_tree_ = false;
  NodeType parent_ = uninitialized_destination;
  bool is_root_ = false;
  NodeListType children_;
};

}}} //end namespace vt::collective::tree

#endif /*INCLUDED_COLLECTIVE_TREE_TREE_H*/
