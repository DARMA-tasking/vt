
#include "config.h"
#include "topos/mapping/mapping.h"
#include "topos/mapping/seed/seed.h"

#include <cstdlib>

namespace vt { namespace mapping {

PhysicalResourceType randomSeedMap(SeedType seed, PhysicalResourceType nres) {
  // @todo: use C++ random number generator instead, these are not thread-safe
  // This must run only on the communication thread
  srand48(seed);
  auto const& map_to = static_cast<PhysicalResourceType>(drand48() * nres);
  return map_to;
}

CoreType randomSeedMapCore(SeedType seed, CoreType ncores) {
  return randomSeedMap(seed, ncores);
}

NodeType randomSeedMapNode(SeedType seed, NodeType nnodes) {
  return randomSeedMap(seed, nnodes);
}

}} /* end namespace vt::mapping */
