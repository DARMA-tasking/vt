
#include "collective/collective_alg.h"

namespace vt { namespace collective {

CollectiveAlg::CollectiveAlg()
  : tree::Tree(tree::tree_cons_tag_t),
    reduce::Reduce(),
    barrier::Barrier()
{ }

}}  // end namespace vt::collective
