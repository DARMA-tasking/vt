
#if !defined INCLUDED_TYPES_TYPE
#define INCLUDED_TYPES_TYPE

#include "configs/debug/debug_masterconfig.h"

#include <cstdint>
#include <functional>

namespace vt {

// Physical identifier types (nodes, cores, workers, etc.)
using PhysicalResourceType = int16_t;
using NodeType = PhysicalResourceType;
using CoreType = PhysicalResourceType;
using WorkerCountType = PhysicalResourceType;
using WorkerIDType = PhysicalResourceType;

// Runtime system entity types
using HandlerType = int32_t;
using SeedType = int64_t;
using EnvelopeDataType = int8_t;
using EventType = uint64_t;
using EpochType = int32_t;
using TagType = int32_t;
using BarrierType = uint64_t;
using CollectiveAlgType = uint64_t;
using RefType = int16_t;
using ByteType = uint64_t;
using BitCountType = int32_t;
using SerialByteType = char;
using ErrorCodeType = int32_t;
using VirtualProxyType = uint64_t;
using VirtualElmOnlyProxyType = uint64_t;
using VirtualElmCountType = int64_t;
using UniqueIndexBitType = uint64_t;
using GroupType = uint64_t;
using MsgSizeType = int32_t;
using PhaseType = uint64_t;

// Action types for attaching a closure to a runtime function
using ActionType = std::function<void()>;
using ActionProxyType = std::function<void(VirtualProxyType)>;
using ActionNodeType = std::function<void(NodeType)>;

}  // end namespace vt

#endif  /*INCLUDED_TYPES_TYPE*/
