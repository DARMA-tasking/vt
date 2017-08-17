
#include "tree.h"
#include "transport.h"

namespace runtime {

void
Tree::setup_tree() {
  if (not set_up_tree) {
    my_node = the_context->get_node();
    num_nodes = the_context->get_num_nodes();

    c1 = my_node*2+1;
    c2 = my_node*2+2;
    has_c1 = c1 < num_nodes;
    has_c2 = c2 < num_nodes;
    num_children = has_c1 + has_c2;

    is_root = my_node == 0;

    if (not is_root) {
      parent = (my_node - 1) / 2;
    }

    set_up_tree = true;
  }
}

} //end namespace runtime

