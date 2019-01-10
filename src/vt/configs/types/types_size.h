/*
//@HEADER
// ************************************************************************
//
//                          types_size.h
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

#if !defined INCLUDED_TYPES_SIZE
#define INCLUDED_TYPES_SIZE

#include "vt/configs/debug/debug_masterconfig.h"
#include "vt/configs/types/types_type.h"
#include "vt/utils/bits/bits_common.h"

namespace vt {

static constexpr BitCountType const
    node_num_bits = BitCounterType<NodeType>::value;
static constexpr BitCountType const
    handler_num_bits = BitCounterType<HandlerType>::value;
static constexpr BitCountType const
    ref_num_bits = BitCounterType<RefType>::value;
static constexpr BitCountType const
    epoch_num_bits = BitCounterType<EpochType>::value;
static constexpr BitCountType const
    tag_num_bits = BitCounterType<TagType>::value;
static constexpr BitCountType const
    group_num_bits = BitCounterType<GroupType>::value;

}  // end namespace vt

#endif  /*INCLUDED_TYPES_SIZE*/
