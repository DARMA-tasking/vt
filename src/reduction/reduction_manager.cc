
#include "config.h"
#include "reduction/reduction_manager.h"
#include "reduction/reduction_state.h"

#include <tuple>
#include <unordered_map>

namespace vt { namespace reduction {

ReductionManager::ReductionManager() : Tree(tree_cons_tag_t) { }

}} /* end namespace vt::reduction */
