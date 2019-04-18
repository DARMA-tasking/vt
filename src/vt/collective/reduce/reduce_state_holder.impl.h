/*
//@HEADER
// ************************************************************************
//
//                          reduce_state_holder.impl.h
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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_REDUCE_STATE_HOLDER_IMPL_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_REDUCE_STATE_HOLDER_IMPL_H

#include "vt/config.h"

namespace vt { namespace collective { namespace reduce {

template <typename T>
/*static*/ bool ReduceStateHolder::exists(
  GroupType group, ReduceIDType const& id
) {
  auto group_iter = state_lookup_<T>.find(group);
  if (group_iter == state_lookup_<T>.end()) {
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

template <typename T>
/*static*/ typename ReduceStateHolder::ReduceStateType<T>&
ReduceStateHolder::find(GroupType group, ReduceIDType const& id) {
  auto group_iter = state_lookup_<T>.find(group);
  vtAssertExpr(group_iter != state_lookup_<T>.end());
  auto id_iter = group_iter->second.find(id);
  vtAssertExpr(id_iter != group_iter->second.end());
  return id_iter->second;
}

template <typename T>
/*static*/ void ReduceStateHolder::erase(
  GroupType group, ReduceIDType const& id
) {
  auto group_iter = state_lookup_<T>.find(group);
  if (group_iter != state_lookup_<T>.end()) {
    auto id_iter = group_iter->second.find(id);
    if (id_iter == group_iter->second.end()) {
      state_lookup_<T>.erase(group_iter);
    } else {
      group_iter->second.erase(id_iter);
      if (group_iter->second.size == 0) {
        state_lookup_<T>.erase(group_iter);
      }
    }
  }
}

template <typename T>
/*static*/ void ReduceStateHolder::insert(
  GroupType group, ReduceIDType const& id, ReduceStateType<T>&& state
) {
  state_lookup_<T>[group].emplace(
    std::piecewise_construct,
    std::forward_as_tuple(id),
    std::forward_as_tuple(std::move(state))
  );
}

template <typename T>
/*static*/ typename ReduceStateHolder::GroupLookupType<T>
ReduceStateHolder::state_lookup_ = {};

}}} /* end namespace vt::collective::reduce */

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_REDUCE_STATE_HOLDER_IMPL_H*/
