
#if !defined INCLUDED_TOPOS_MAPPING_MAPPING_FUNCTION_H
#define INCLUDED_TOPOS_MAPPING_MAPPING_FUNCTION_H

#include "vt/config.h"
#include "vt/topos/index/index.h"

namespace vt { namespace mapping {

template <typename IndexT>
using ActiveMapTypedFnType = NodeType(IndexT*, IndexT*, NodeType);
using ActiveMapFnPtrType = NodeType(*)(
  index::BaseIndex*, index::BaseIndex*, NodeType
);

using ActiveSeedMapFnType = NodeType(SeedType, NodeType);
using ActiveSeedMapFnPtrType = ActiveSeedMapFnType*;
using CollectionMapFnType = ActiveMapFnPtrType;

}} /* end namespace vt::mapping */

#endif /*INCLUDED_TOPOS_MAPPING/MAPPING_FUNCTION_H*/
