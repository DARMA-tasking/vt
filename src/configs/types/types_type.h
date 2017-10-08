
#if !defined INCLUDED_TYPES_TYPE
#define INCLUDED_TYPES_TYPE

#include <cstdint>
#include <functional>

#include "configs/debug/debug_masterconfig.h"

namespace vt {

using PhysicalResourceType = uint16_t;
using NodeType = PhysicalResourceType;
using CoreType = PhysicalResourceType;
using SeedType = int64_t;
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
using VirtualProxyType = uint64_t;
using SerialByteType = char;
using WorkerCountType = int32_t;

}  // end namespace vt

#endif  /*INCLUDED_TYPES_TYPE*/
