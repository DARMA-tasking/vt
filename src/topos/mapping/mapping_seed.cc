
#include "config.h"
#include "mapping.h"
#include "mapping_seed.h"

#include <cstdlib>

namespace vt { namespace mapping {

template <typename PhysicalT>
PhysicalT randomSeedMap(SeedType seed, PhysicalT nres) {
  srand48(seed);
  auto const& map_to = static_cast<PhysicalT>(drand48() * nres);
  return map_to;
}

CoreType randomSeedMapCore(SeedType seed, CoreType ncores) {
  return randomSeedMap(seed, ncores);
}

NodeType randomSeedMapNode(SeedType seed, NodeType nnodes) {
  return randomSeedMap(seed, nnodes);
}

}} /* end namespace vt::mapping */
