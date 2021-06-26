/*
//@HEADER
// *****************************************************************************
//
//                                rdma_common.h
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

#if !defined INCLUDED_VT_RDMA_RDMA_COMMON_H
#define INCLUDED_VT_RDMA_RDMA_COMMON_H

#include "vt/config.h"
#include "vt/messaging/message.h"
#include "vt/utils/bits/bits_common.h"

#include "vt/rdma/rdma_types.h"

#include <tuple>

namespace vt { namespace rdma {

using RDMA_IdentifierType = int32_t;

static constexpr RDMA_IdentifierType const first_rdma_identifier = 1;
static constexpr RDMA_IdentifierType const uninitialized_rdma_identifier = -1;
static constexpr TagType const first_rdma_channel_tag = 1;

// 64 bits: RDMA handle
//   int64_t handle/handler : [24..56]
//   int64_t node : 16 [7..23]
//   int64_t op_type : 4 [3]
//   int64_t is_HandlerTypeype : 1 [2]
//   int64_t is_collective : 1 [1]
//   int64_t is_sized : 1 [0]

enum Type {
  Get = 0,
  Put = 1,
  GetOrPut = 2,
  Uninitialized = 3
};

static constexpr BitCountType const rdma_type_num_bits = 4;
static constexpr BitCountType const rdma_sized_num_bits = 1;
static constexpr BitCountType const rdma_collective_num_bits = 1;
static constexpr BitCountType const rdma_hander_type_num_bits = 1;
static constexpr BitCountType const rdma_identifier_num_bits =
  BitCounterType<RDMA_IdentifierType>::value;

enum Bits {
  Sized       = 0,
  Collective  = Bits::Sized       + rdma_sized_num_bits,
  HandlerType = Bits::Collective  + rdma_collective_num_bits,
  OpType      = Bits::HandlerType + rdma_hander_type_num_bits,
  Identifier  = Bits::OpType      + rdma_type_num_bits,
  Node        = Bits::Identifier  + rdma_identifier_num_bits
};

using RDMA_OpType = int64_t;

static constexpr RDMA_OpType const no_rdma_op = -1;

template <typename MsgType>
using ActiveTypedGetFunctionType = RDMA_GetType(*)(MsgType*, ByteType, ByteType, TagType, bool);
template <typename MsgType>
using ActiveTypedPutFunctionType = void(*)(MsgType*, RDMA_PtrType, ByteType, ByteType, TagType, bool);

using ActiveGetFunctionType = RDMA_GetType(*)(BaseMessage*, ByteType, ByteType, TagType, bool);
using ActivePutFunctionType = void(*)(BaseMessage*, RDMA_PtrType, ByteType, ByteType, TagType, bool);

using RDMA_PtrContinuationType = std::function<void(RDMA_PtrType)>;
using RDMA_RecvType = std::function<void(void* ptr, size_t num_bytes)>;

using RDMA_NumElemsType = int64_t;
using RDMA_BlockElmRangeType = std::tuple<RDMA_BlockType,RDMA_ElmType,RDMA_ElmType>;
using RDMA_BlockMapType = std::function<NodeType(RDMA_BlockType,RDMA_BlockType)>;
using RDMA_ElmMapType = std::function<RDMA_BlockElmRangeType(RDMA_ElmType,RDMA_ElmType,RDMA_BlockType)>;

static constexpr Type uninitialized_rdma_type = Type::Uninitialized;
static constexpr ByteType rdma_default_byte_size = sizeof(char);

}} //end namespace vt::rdma

#define PRINT_CHANNEL_TYPE(rdma_op_type) (                              \
  rdma_op_type == vt::rdma::Type::Get ? "rdma::Get" : (            \
    rdma_op_type == vt::rdma::Type::Put ? "rdma::Put" : (          \
      rdma_op_type == vt::rdma::Type::GetOrPut ? "rdma::GetorPut"  \
      : (                                                               \
        rdma_op_type == vt::rdma::Type::Uninitialized ?            \
        "rdma::Uninitialized" : "Error: unknown rdma::Type"             \
      )                                                                 \
    )                                                                   \
  )                                                                     \
)

#endif /*INCLUDED_VT_RDMA_RDMA_COMMON_H*/
