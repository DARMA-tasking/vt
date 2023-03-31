/*
//@HEADER
// *****************************************************************************
//
//                              types_sentinels.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_CONFIGS_TYPES_TYPES_SENTINELS_H
#define INCLUDED_VT_CONFIGS_TYPES_TYPES_SENTINELS_H

#include "vt/configs/types/types_type.h"
#include "vt/configs/types/types_rdma.h"

namespace vt {

// Physical identifier sentinel values (nodes etc.)
static constexpr NodeType const uninitialized_destination          = static_cast<NodeType>(0xFFFF);

static constexpr PhysicalResourceType const no_workers [[deprecated]]   = static_cast<PhysicalResourceType>(0xFFFF);

// Runtime default `empty' sentinel value
static constexpr uint64_t const u64empty = 0xFFFFFFFFFFFFFFFF;
static constexpr uint32_t const u32empty = 0xFEEDFEED;
static constexpr int64_t  const s64empty = 0xFFFFFFFFFFFFFFFF;
static constexpr int32_t  const s32empty = 0xFEEDFEED;

// Runtime identifier sentinel values
static constexpr int const num_check_actions                       = -1;
static constexpr TagType const no_tag                              = -1;
static constexpr EventType const no_event                          = u64empty;
static constexpr BarrierType const no_barrier                      = u64empty;
static constexpr RDMA_HandleType const no_rdma_handle              = -1;
static constexpr ByteType const no_byte                            = u64empty;
static constexpr ByteType const no_offset                          = u64empty;
static constexpr auto no_action                                    = nullptr;
static constexpr RDMA_PtrType const no_rdma_ptr                    = nullptr;
static constexpr VirtualProxyType const no_vrt_proxy               = u64empty;
static constexpr HandlerType const uninitialized_handler           = -1;
static constexpr RDMA_HandlerType const uninitialized_rdma_handler = -1;
static constexpr RefType const not_shared_message                  = 65535;
static constexpr RDMA_BlockType const no_rdma_block                = -1;
static constexpr SeedType const no_seed                            = -1;
static constexpr VirtualElmCountType const no_elms                 = -1;
static constexpr TagType const local_rdma_op_tag                   = s32empty;
static constexpr GroupType const no_group                          = u64empty;
static constexpr GroupType const default_group                     = 0xFFFFFFFF;
static constexpr PhaseType const fst_lb_phase                      = 0;
static constexpr PhaseType const no_lb_phase                       = u64empty;
static constexpr PipeType const no_pipe                            = u64empty;
static constexpr ObjGroupProxyType const no_obj_group              = u64empty;
static constexpr SequentialIDType const no_seq_id                  = u64empty;
static constexpr SequentialIDType const first_seq_id               = 1;
static constexpr PriorityType const no_priority                    = 0;
static constexpr PriorityLevelType const no_priority_level         = 0;
static constexpr ThreadIDType const no_thread_id                   = 0;

}  // end namespace vt

#endif  /*INCLUDED_VT_CONFIGS_TYPES_TYPES_SENTINELS_H*/
