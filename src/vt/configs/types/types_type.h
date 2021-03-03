/*
//@HEADER
// *****************************************************************************
//
//                                 types_type.h
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_TYPES_TYPE
#define INCLUDED_TYPES_TYPE

#include <cstdint>
#include <functional>

namespace vt {

/** \file */

// Physical identifier types (nodes, cores, workers, etc.)
using PhysicalResourceType    = int16_t;
/// Used to hold the current node/rank or the number of nodes
using NodeType                = PhysicalResourceType;
/// Used to hold a core ID
using CoreType                = PhysicalResourceType;
/// Used to hold the number of workers on a node
using WorkerCountType         = PhysicalResourceType;
/// Used to hold the ID of a worker on a node
using WorkerIDType            = PhysicalResourceType;

// Runtime system entity types
/// Used to hold a handler ID which identifier a function pointer/context
using HandlerType             = int64_t;
/// Used to hold a seed for random generation
using SeedType                = int64_t;
/// Used to hold the control bits in an envelope
using EnvelopeDataType        = int8_t;
/// Used to hold a local/remote event to wait for completion
using EventType               = uint64_t;
/// Used to hold a sequential identifier for ordered/stateful operations
using SequentialIDType        = uint64_t;
/// Used to hold an epoch for termination detection
using EpochType               = uint64_t;
/// Used to hold an tag, e.g., on messages or reduces
using TagType                 = int32_t;
/// Used to identify a specific barrier
using BarrierType             = uint64_t;
/// Used to identify a collective operation
using CollectiveAlgType       = uint64_t;
/// Used to hold the reference count for messages
using RefType                 = int16_t;
/// Used to store some number of bytes
using ByteType                = uint64_t;
/// Used to store the number of bits in a field
using BitCountType            = int32_t;
/// Used to store the number of bits for serialization
using SerialByteType          = char;
/// Used to store an error code
using ErrorCodeType           = int32_t;
/// Used to hold an identifier for a collection or other proxy
using VirtualProxyType        = uint64_t;
/// Used to hold an identifier for an element in a collection
using VirtualElmOnlyProxyType = uint64_t;
/// Used to hold the count of elements in a collection
using VirtualElmCountType     = int64_t;
/// Used for mapping between index to contiguous bits
using UniqueIndexBitType      = uint64_t;
/// Used for hold an identifier for a group
using GroupType               = uint64_t;
/// Used for hold the size of a message
using MsgSizeType             = int64_t;
/// Used for hold a phase for load balancing
using PhaseType               = uint64_t;
/// Used for hold a sub-phase for load balancing
using SubphaseType            = uint64_t;
/// Used for hold the identifier for a pipe (callbacks)
using PipeType                = uint64_t;
/// Used for hold the proxy ID for an objgroup
using ObjGroupProxyType       = uint64_t;
/// Used for hold the priority of a message
using PriorityType            = uint16_t;
/// Used for hold the level for a priority of a message
using PriorityLevelType       = uint8_t;
/// Used for hold a unique ID for each component
using ComponentIDType         = uint32_t;

// Action types for attaching a closure to a runtime function
/// Used for generically store an action to perform
using ActionType              = std::function<void()>;
/// Used for generically store an action that requires a proxy
using ActionProxyType         = std::function<void(VirtualProxyType)>;
/// Used for generically store an action that requires a node
using ActionNodeType          = std::function<void(NodeType)>;

}  // end namespace vt

#endif  /*INCLUDED_TYPES_TYPE*/
