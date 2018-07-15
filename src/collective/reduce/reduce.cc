
#include "config.h"
#include "collective/reduce/reduce.h"
#include "collective/reduce/reduce_state.h"

#include <tuple>
#include <unordered_map>

namespace vt { namespace collective { namespace reduce {

Reduce::Reduce()
  : tree::Tree(tree::tree_cons_tag_t)
{
  debug_print(
    reduce, node,
    "Reduce constructor: children={}, parent={}\n",
    getNumChildren(), getParent()
  );
}

Reduce::Reduce(collective::tree::Tree* in_tree)
  : tree::Tree(*in_tree)
{
  debug_print(
    reduce, node,
    "Reduce constructor: children={}, parent={}\n",
    getNumChildren(), getParent()
  );
}

}}} /* end namespace vt::collective::reduce */
