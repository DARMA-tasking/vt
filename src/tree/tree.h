
#if ! defined __RUNTIME_TRANSPORT_TREE__
#define __RUNTIME_TRANSPORT_TREE__

#include "common.h"

namespace runtime {

struct Tree {
  void setup_tree();

  bool set_up_tree = false;
  NodeType c1 = -1, c2 = -1, parent = -1;
  NodeType num_children = 0, my_node = 0, num_nodes = 0;
  bool has_c1 = false, has_c2 = false, is_root = false;
};

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_TREE__*/
