/*
//@HEADER
// *****************************************************************************
//
//                               state_holder.cc
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

#include "vt/config.h"
#include "state_holder.h"
#include "vt/configs/error/hard_error.h"
#include "vt/configs/error/config_assert.h"

namespace vt::collective::reduce::allreduce {

size_t
getNextIdImpl(StateHolder::StatesVec& states, size_t idx) {
  size_t id = u64empty;

  vt_debug_print(
    terse, allreduce, "getNextIdImpl idx={} size={} \n", idx, states.size());

  for (; idx < states.size(); ++idx) {
    auto& state = states.at(idx);
    if (not state or not state->active_) {
      id = idx;
      break;
    }
  }

  if (id == u64empty) {
    id = states.size();
  }


  return id;
}

size_t StateHolder::getNextID(detail::StrongVrtProxy proxy) {
  auto& [idx, states] = active_coll_states_[proxy.get()];

  auto current_idx =  getNextIdImpl(states, idx);
  idx = current_idx + 1;

  return current_idx;
}

size_t StateHolder::getNextID(detail::StrongObjGroup proxy) {
  auto& [idx, states] = active_obj_states_[proxy.get()];

  auto current_idx = getNextIdImpl(states, idx);
  idx = current_idx + 1;

  return current_idx;
}

size_t StateHolder::getNextID(detail::StrongGroup group) {
  auto& [idx, states] = active_grp_states_[group.get()];

  auto current_idx = getNextIdImpl(states, idx);

  idx = current_idx + 1;
  return current_idx;
}

static inline void
clearSingleImpl(StateHolder::StatesVec& states, size_t idx) {
  auto const num_states = states.size();
  vtAssert(
    num_states > idx,
    fmt::format(
      "Attempting to access state {} with total numer of states {}!", idx,
      num_states));

  states.at(idx).reset();
}

void StateHolder::clearSingle(detail::StrongVrtProxy proxy, size_t idx) {
  auto& [_, states] = active_coll_states_[proxy.get()];

  clearSingleImpl(states, idx);
}

void StateHolder::clearSingle(detail::StrongObjGroup proxy, size_t idx) {
  auto& [_, states] = active_obj_states_[proxy.get()];

  clearSingleImpl(states, idx);
}

void StateHolder::clearSingle(detail::StrongGroup group, size_t idx) {
  auto& [_, states] = active_grp_states_[group.get()];

  clearSingleImpl(states, idx);
}

void StateHolder::clearAll(detail::StrongVrtProxy proxy) {
  active_coll_states_.erase(proxy.get());
}

void StateHolder::clearAll(detail::StrongObjGroup proxy) {
  active_obj_states_.erase(proxy.get());
}

void StateHolder::clearAll(detail::StrongGroup group) {
  active_grp_states_.erase(group.get());
}

} // namespace vt::collective::reduce::allreduce
