
#if ! defined __RUNTIME_TRANSPORT_TREE__
#define __RUNTIME_TRANSPORT_TREE__

#include "config.h"

namespace vt {

static struct TreeConstructTag { } tree_cons_tag_t { };

struct Tree {
  explicit Tree(TreeConstructTag);

  void setupTree();
  NodeType getParent() const;
  NodeType getNumChildren() const;
  bool isRoot() const;

private:
  bool set_up_tree_ = false;
  NodeType c1_ = -1, c2_ = -1, parent_ = -1;
  NodeType num_children_ = 0, my_node_ = 0, num_nodes_ = 0;
  bool has_c1_ = false, has_c2_ = false, is_root_ = false;
};

} //end namespace vt

#endif /*__RUNTIME_TRANSPORT_TREE__*/
