
#if ! defined __RUNTIME_TRANSPORT_COMMON_TYPE_SIZES__
#define __RUNTIME_TRANSPORT_COMMON_TYPE_SIZES__

#include "utils/debug/debug_masterconfig.h"
#include "common_types.h"
#include "utils/bits/bits_common.h"

namespace vt {

static constexpr BitCountType const node_num_bits = BitCounterType<NodeType>::value;
static constexpr BitCountType const handler_num_bits = BitCounterType<HandlerType>::value;
static constexpr BitCountType const ref_num_bits = BitCounterType<RefType>::value;
static constexpr BitCountType const epoch_num_bits = BitCounterType<EpochType>::value;
static constexpr BitCountType const tag_num_bits = BitCounterType<TagType>::value;

} //end namespace vt

#endif /*__RUNTIME_TRANSPORT_COMMON_TYPE_SIZES__*/
