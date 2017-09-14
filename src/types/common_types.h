
#if ! defined __RUNTIME_TRANSPORT_COMMON_TYPES__
#define __RUNTIME_TRANSPORT_COMMON_TYPES__

#include "utils/debug/debug_masterconfig.h"

#include <cstdint>
#include <functional>

namespace vt {

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

} //end namespace vt

#endif /*__RUNTIME_TRANSPORT_EVENT_COMMON_TYPES__*/
