
#if !defined INCLUDED_TOPOS_MAPPING_MAPPING_SEED_H
#define INCLUDED_TOPOS_MAPPING_MAPPING_SEED_H

#include "config.h"
#include "topos/mapping/mapping.h"

#include <cstdlib>

namespace vt { namespace mapping {

CoreType randomSeedMapCore(SeedType seed, CoreType ncores);
NodeType randomSeedMapNode(SeedType seed, NodeType nnodes);

}} /* end namespace vt::mapping */

#endif /*INCLUDED_TOPOS_MAPPING/MAPPING_SEED_H*/
