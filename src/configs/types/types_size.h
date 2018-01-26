
#if !defined INCLUDED_TYPES_SIZE
#define INCLUDED_TYPES_SIZE

#include "configs/debug/debug_masterconfig.h"
#include "types_type.h"
#include "utils/bits/bits_common.h"

namespace vt {

static constexpr BitCountType const
    node_num_bits = BitCounterType<NodeType>::value;
static constexpr BitCountType const
    handler_num_bits = BitCounterType<HandlerType>::value;
static constexpr BitCountType const
    ref_num_bits = BitCounterType<RefType>::value;
static constexpr BitCountType const
    epoch_num_bits = BitCounterType<EpochType>::value;
static constexpr BitCountType const
    tag_num_bits = BitCounterType<TagType>::value;
static constexpr BitCountType const
    group_num_bits = BitCounterType<GroupType>::value;

}  // end namespace vt

#endif  /*INCLUDED_TYPES_SIZE*/
