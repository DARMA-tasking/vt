
#if !defined INCLUDED_LB_LB_TYPES_H
#define INCLUDED_LB_LB_TYPES_H

#include <cstdlib>
#include <cstdint>

namespace vt {

using LBEntityType = uint64_t;
using LBPhaseType = int64_t;

static constexpr LBPhaseType const fst_phase = 0;
static constexpr LBPhaseType const no_phase = -1;

} /* end namespace vt */

#endif /*INCLUDED_LB_LB_TYPES_H*/
