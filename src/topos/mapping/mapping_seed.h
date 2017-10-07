
#if !defined __RUNTIME_TRANSPORT_MAPPING_SEED__
#define __RUNTIME_TRANSPORT_MAPPING_SEED__

#include "config.h"
#include "mapping.h"

#include <cstdlib>

namespace vt { namespace mapping {

CoreType randomSeedMapCore(SeedType seed, CoreType nnodes);
NodeType randomSeedMapNode(SeedType seed, NodeType nnodes);

}} /* end namespace vt::mapping */

#endif /*__RUNTIME_TRANSPORT_MAPPING_SEED__*/
