
#if ! defined __RUNTIME_TRANSPORT_COMMON_TYPE_SIZES__
#define __RUNTIME_TRANSPORT_COMMON_TYPE_SIZES__

#include "config.h"
#include "common_types.h"
#include "bit_common.h"

namespace runtime {

static constexpr bit_count_t const node_num_bits = bit_counter_t<NodeType>::value;
static constexpr bit_count_t const handler_num_bits = bit_counter_t<HandlerType>::value;
static constexpr bit_count_t const ref_num_bits = bit_counter_t<ref_t>::value;
static constexpr bit_count_t const epoch_num_bits = bit_counter_t<EpochType>::value;
static constexpr bit_count_t const tag_num_bits = bit_counter_t<TagType>::value;

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_COMMON_TYPE_SIZES__*/
