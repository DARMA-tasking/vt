
#if ! defined __RUNTIME_TRANSPORT_COMMON_TYPE_SIZES__
#define __RUNTIME_TRANSPORT_COMMON_TYPE_SIZES__

#include "config.h"
#include "common_types.h"
#include "bit_common.h"

namespace runtime {

static constexpr BitCountType const node_num_bits = BitCounterType<NodeType>::value;
static constexpr BitCountType const handler_num_bits = BitCounterType<HandlerType>::value;
static constexpr BitCountType const ref_num_bits = BitCounterType<RefType>::value;
static constexpr BitCountType const epoch_num_bits = BitCounterType<EpochType>::value;
static constexpr BitCountType const tag_num_bits = BitCounterType<TagType>::value;

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_COMMON_TYPE_SIZES__*/
