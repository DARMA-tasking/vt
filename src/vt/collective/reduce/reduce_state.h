/*
//@HEADER
// ************************************************************************
//
//                          reduce_state.h
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

#if !defined INCLUDED_COLLECTIVE_REDUCE_REDUCE_STATE_H
#define INCLUDED_COLLECTIVE_REDUCE_REDUCE_STATE_H

#include "vt/config.h"
#include "vt/collective/reduce/reduce_msg.h"
#include "vt/messaging/message.h"

#include <vector>
#include <cstdint>

namespace vt { namespace collective { namespace reduce {

struct ReduceState {
  using ReduceNumType = int32_t;
  using ReduceVecType = std::vector<MsgSharedPtr<ReduceMsg>>;

  ReduceState(
    TagType const& in_tag_, EpochType const& in_epoch_,
    ReduceNumType const& in_num_contrib
  ) : tag_(in_tag_), epoch_(in_epoch_), num_contrib_(in_num_contrib)
  { }

  ReduceVecType msgs               = {};
  TagType tag_                     = no_tag;
  EpochType epoch_                 = no_epoch;
  ReduceNumType num_contrib_       = 1;
  ReduceNumType num_local_contrib_ = 0;
  HandlerType combine_handler_     = uninitialized_handler;
  NodeType reduce_root_            = uninitialized_destination;
};

}}} /* end namespace vt::collective::reduce */

#endif /*INCLUDED_COLLECTIVE_REDUCE_REDUCE_STATE_H*/
