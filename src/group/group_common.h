
#if !defined INCLUDED_GROUP_GROUP_COMMON_H
#define INCLUDED_GROUP_GROUP_COMMON_H

#include <cstdlib>

namespace vt { namespace group {

using GroupIDType = uint32_t;

static constexpr GroupIDType const initial_group_id = 0;
static constexpr GroupIDType const no_group_id = 0xFFFFFFFF;

using RemoteOperationIDType = size_t;

static constexpr RemoteOperationIDType const no_op_id = -1;

}} /* end namespace vt::group */

#endif /*INCLUDED_GROUP_GROUP_COMMON_H*/
