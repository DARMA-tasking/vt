
#if !defined INCLUDED_COLLECTIVE_TREE_TREE_H
#define INCLUDED_COLLECTIVE_TREE_TREE_H

#include "config.h"

#include <vector>
#include <functional>

namespace vt { namespace collective { namespace tree {

static struct DefaultTreeConstructTag { } tree_cons_tag_t { };

struct Tree {
  using NodeListType = std::vector<NodeType>;
  using OperationType = std::function<void(NodeType)>;

  explicit Tree(DefaultTreeConstructTag);
  explicit Tree(NodeListType const& in_children);

  void setupTree();
  NodeType getParent() const;
  NodeType getNumChildren() const;
  bool isRoot() const;
  NodeListType const& getChildren() const;
  void foreachChild(OperationType op) const;

private:
  bool set_up_tree_ = false;
  NodeType parent_ = uninitialized_destination;
  bool is_root_ = false;
  NodeListType children_;
};

}}} //end namespace vt::collective::tree

#endif /*INCLUDED_COLLECTIVE_TREE_TREE_H*/
