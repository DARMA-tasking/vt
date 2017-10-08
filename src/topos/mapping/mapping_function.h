
#if !defined __RUNTIME_TRANSPORT_MAPPING_FUNCTION__
#define __RUNTIME_TRANSPORT_MAPPING_FUNCTION__

#include "config.h"
#include "topos/index/index.h"

namespace vt { namespace mapping {

template <typename IndexT>
using ActiveMapTypedFnType = NodeType(IndexT*, IndexT*, NodeType);
using ActiveMapFnPtrType = NodeType(*)(
  index::BaseIndex*, index::BaseIndex*, NodeType
);

using ActiveSeedMapFnType = NodeType(SeedType, NodeType);
using ActiveSeedMapFnPtrType = ActiveSeedMapFnType*;

}} /* end namespace vt::mapping */

#endif /*__RUNTIME_TRANSPORT_MAPPING_FUNCTION__*/
