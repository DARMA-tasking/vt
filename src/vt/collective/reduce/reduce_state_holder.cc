/*
//@HEADER
// *****************************************************************************
//
//                           reduce_state_holder.cc
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

#include "vt/config.h"
#include "vt/collective/reduce/reduce_state_holder.h"

namespace vt { namespace collective { namespace reduce {

bool ReduceStateHolder::exists(
  GroupType group, ReduceIDType const& id
) {
  auto group_iter = state_lookup_.find(group);
  if (group_iter == state_lookup_.end()) {
    return false;
  } else {
    auto id_iter = group_iter->second.find(id);
    if (id_iter == group_iter->second.end()) {
      return false;
    } else {
      return true;
    }
  }
}

ReduceStateHolder::ReduceStateType&
ReduceStateHolder::find(GroupType group, ReduceIDType const& id) {
  auto group_iter = state_lookup_.find(group);
  vtAssertExpr(group_iter != state_lookup_.end());
  auto id_iter = group_iter->second.find(id);
  vtAssertExpr(id_iter != group_iter->second.end());
  return id_iter->second;
}

void ReduceStateHolder::erase(
  GroupType group, ReduceIDType const& id
) {
  auto group_iter = state_lookup_.find(group);
  if (group_iter != state_lookup_.end()) {
    auto id_iter = group_iter->second.find(id);
    if (id_iter == group_iter->second.end()) {
      state_lookup_.erase(group_iter);
    } else {
      group_iter->second.erase(id_iter);
      if (group_iter->second.size() == 0) {
        state_lookup_.erase(group_iter);
      }
    }
  }
}

void ReduceStateHolder::insert(
  GroupType group, ReduceIDType const& id, ReduceStateType&& state
) {
  state_lookup_[group].emplace(
    std::piecewise_construct,
    std::forward_as_tuple(id),
    std::forward_as_tuple(std::move(state))
  );
}

}}} /* end namespace vt::collective::reduce */
