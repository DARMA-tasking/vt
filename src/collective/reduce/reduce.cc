
#include "config.h"
#include "collective/reduce/reduce.h"
#include "collective/reduce/reduce_state.h"

#include <tuple>
#include <unordered_map>

namespace vt { namespace collective { namespace reduce {

Reduce::Reduce()
  : tree::Tree(tree::tree_cons_tag_t)
{ }

}}} /* end namespace vt::collective::reduce */
