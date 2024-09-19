/*
//@HEADER
// *****************************************************************************
//
//                                 types_rdma.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_CONFIGS_TYPES_TYPES_RDMA_H
#define INCLUDED_VT_CONFIGS_TYPES_TYPES_RDMA_H

#include "vt/configs/types/types_type.h"

#include <cstdint>
#include <functional>
#include <tuple>

namespace vt {

using RDMA_PtrType                = std::byte *;
using RDMA_ElmType                = uint64_t;
using RDMA_BlockType              = int64_t;
using RDMA_HandleType             = int64_t;
using RDMA_HandlerType            = int64_t;
using RDMA_GetType                = std::tuple<RDMA_PtrType, ByteType>;
using RDMA_PutRetType             = RDMA_GetType;
using RDMA_ContinuationType       = std::function<void(RDMA_GetType)>;
using RDMA_ContinuationDeleteType = std::function<
  void(RDMA_GetType, ActionType)
>;
using RDMA_PutSerialize = std::function<RDMA_PutRetType(RDMA_PutRetType)>;

}  // end namespace vt

#endif /*INCLUDED_VT_CONFIGS_TYPES_TYPES_RDMA_H*/
