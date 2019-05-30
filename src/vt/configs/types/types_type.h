/*
//@HEADER
// ************************************************************************
//
//                          types_type.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_TYPES_TYPE
#define INCLUDED_TYPES_TYPE

#include "vt/configs/debug/debug_masterconfig.h"

#include <cstdint>
#include <functional>

namespace vt {

// Physical identifier types (nodes, cores, workers, etc.)
using PhysicalResourceType    = int16_t;
using NodeType                = PhysicalResourceType;
using CoreType                = PhysicalResourceType;
using WorkerCountType         = PhysicalResourceType;
using WorkerIDType            = PhysicalResourceType;

// Runtime system entity types
using HandlerType             = uint64_t;
using SeedType                = int64_t;
using EnvelopeDataType        = int8_t;
using EventType               = uint64_t;
using SequentialIDType        = uint64_t;
using EpochType               = uint64_t;
using TagType                 = int32_t;
using BarrierType             = uint64_t;
using CollectiveAlgType       = uint64_t;
using RefType                 = int16_t;
using ByteType                = uint64_t;
using BitCountType            = int32_t;
using SerialByteType          = char;
using ErrorCodeType           = int32_t;
using VirtualProxyType        = uint64_t;
using VirtualElmOnlyProxyType = uint64_t;
using VirtualElmCountType     = int64_t;
using UniqueIndexBitType      = uint64_t;
using GroupType               = uint64_t;
using MsgSizeType             = int32_t;
using PhaseType               = uint64_t;
using PipeType                = uint64_t;
using ObjGroupProxyType       = uint64_t;

// Action types for attaching a closure to a runtime function
using ActionType              = std::function<void()>;
using ActionProxyType         = std::function<void(VirtualProxyType)>;
using ActionNodeType          = std::function<void(NodeType)>;

}  // end namespace vt

#endif  /*INCLUDED_TYPES_TYPE*/
