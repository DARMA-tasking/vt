
#if ! defined __RUNTIME_TRANSPORT_COMMON_TYPES__
#define __RUNTIME_TRANSPORT_COMMON_TYPES__

#include "config.h"

#include <cstdint>
#include <functional>

namespace runtime {

using NodeType = uint16_t;
using HandlerType = int32_t;
using EnvelopeDataType = int8_t;
using EventType = uint64_t;
using EpochType = int32_t;
using TagType = int32_t;
using BarrierType = uint64_t;
using RefType = int16_t;
using ByteType = uint64_t;
using BitCountType = int32_t;
using ActionType = std::function<void()>;
using RDMA_PtrType = void*;
using RDMA_ElmType = int64_t;
using RDMA_BlockType = int64_t;
using RDMA_HandleType = int64_t;
using RDMA_HandlerType = int64_t;
using rdma_get_t = std::tuple<RDMA_PtrType, ByteType>;
using rdma_continuation_t = std::function<void(rdma_get_t)>;
using rdma_continuation_del_t = std::function<void(rdma_get_t, ActionType)>;

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_EVENT_COMMON_TYPES__*/
