
#if !defined INCLUDED_TOPOS_MAPPING
#define INCLUDED_TOPOS_MAPPING

#include "config.h"
#include "topos/mapping/mapping_function.h"
#include "topos/mapping/seed/seed.h"
#include "topos/index/index.h"
#include "registry/auto_registry_map.h"

#include <functional>

namespace vt { namespace mapping {

// General mapping functions: maps indexed collections to hardware
template <typename IndexType>
using MapType = PhysicalResourceType(*)(IndexType*, PhysicalResourceType);

template <typename IndexType>
using NodeMapType = MapType<IndexType>;
template <typename IndexType>
using CoreMapType = MapType<IndexType>;

// Dense index mapping functions: maps dense index, with dense regions size, to
// hardware
template <typename IndexType>
using DenseMapType = PhysicalResourceType(*)(
  IndexType*, IndexType*, PhysicalResourceType
);

template <typename IndexType>
using DenseNodeMapType = DenseMapType<IndexType>;
template <typename IndexType>
using DenseCoreMapType = DenseMapType<IndexType>;

// Seed mapping functions for singleton mapping to hardware
using SeedMapType = PhysicalResourceType(*)(SeedType, PhysicalResourceType);

using NodeSeedMapType = SeedMapType;
using CoreSeedMapType = SeedMapType;

}}  // end namespace vt::location

#endif  /*INCLUDED_TOPOS_MAPPING*/
