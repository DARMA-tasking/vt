
#if !defined __RUNTIME_TRANSPORT_MAPPING_FUNCTION__
#define __RUNTIME_TRANSPORT_MAPPING_FUNCTION__

#include "config.h"
#include "topos/index/index.h"

namespace vt { namespace mapping {

using SimpleMapFunctionType = NodeType(*)(
  index::BaseIndex*, index::BaseIndex*, NodeType
);

template <typename IndexT>
using ActiveMapFunctionType = NodeType(IndexT*, IndexT*, NodeType);

using SimpleSeedMapFunctionType = NodeType(*)(SeedType, NodeType);
using ActiveSeedMapFunctionType = NodeType(SeedType, NodeType);

}} /* end namespace vt::mapping */

#endif /*__RUNTIME_TRANSPORT_MAPPING_FUNCTION__*/
