
#if ! defined __RUNTIME_TRANSPORT_COMMON_TYPES__
#define __RUNTIME_TRANSPORT_COMMON_TYPES__

#include "config.h"

#include <cstdint>
#include <functional>

namespace runtime {

using NodeType = uint16_t;
using HandlerType = int32_t;
using EnvelopeDataType = int8_t;
using event_t = uint64_t;
using EpochType = int32_t;
using tag_t = int32_t;
using barrier_t = uint64_t;
using ref_t = int16_t;
using byte_t = uint64_t;
using bit_count_t = int32_t;
using action_t = std::function<void()>;
using rdma_ptr_t = void*;
using rdma_elm_t = int64_t;
using rdma_block_t = int64_t;
using rdma_handle_t = int64_t;
using rdma_handler_t = int64_t;
using rdma_get_t = std::tuple<rdma_ptr_t, byte_t>;
using rdma_continuation_t = std::function<void(rdma_get_t)>;
using rdma_continuation_del_t = std::function<void(rdma_get_t, action_t)>;

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_EVENT_COMMON_TYPES__*/
