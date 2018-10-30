
#if !defined INCLUDED_LB_LB_TYPES_INTERNAL_H
#define INCLUDED_LB_LB_TYPES_INTERNAL_H

#include "config.h"
#include "lb/lb_types.h"
#include "lb/instrumentation/entry.h"

#include <vector>
#include <unordered_map>

namespace vt { namespace lb {

using EntryType = instrumentation::Entry;
using EntryListType = std::vector<EntryType>;
using ContainerType = std::unordered_map<LBEntityType, EntryListType>;
using ProcContainerType = std::unordered_map<NodeType, ContainerType>;

}} /* end namespace lb */

#endif /*INCLUDED_LB_LB_TYPES_INTERNAL_H*/
