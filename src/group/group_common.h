
#if !defined INCLUDED_GROUP_GROUP_COMMON_H
#define INCLUDED_GROUP_GROUP_COMMON_H

#include <cstdlib>

namespace vt { namespace group {

using GroupIDType = uint32_t;

static constexpr GroupIDType const initial_group_id = 0;
static constexpr GroupIDType const no_group_id = 0xFFFFFFFF;

}} /* end namespace vt::group */

#endif /*INCLUDED_GROUP_GROUP_COMMON_H*/
