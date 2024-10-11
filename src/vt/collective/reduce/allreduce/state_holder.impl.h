/*
//@HEADER
// *****************************************************************************
//
//                             state_holder.impl.h
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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_STATE_HOLDER_IMPL_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_STATE_HOLDER_IMPL_H

#include "state_holder.h"
#include "vt/configs/debug/debug_print.h"

#include <type_traits>

namespace vt::collective::reduce::allreduce {

template <
  typename StateT, typename DataT, typename Scalar, typename StateContainerT>
static inline auto& getState(StateContainerT& states, size_t idx, uint64_t id) {
  if (!states.at(idx)) {
    vt_debug_print(
      verbose, allreduce,
      "Creating new allreduce state for id={:x} for idx={} "
      "Scalar_typeid={} DataT_typeid={}\n",
      id, idx, typeid(Scalar).name(), typeid(DataT).name());
    states.at(idx) = std::make_unique<StateT>();
  }

  auto* ptr =
    dynamic_cast<StateT*>(states.at(idx).get());
  vtAssert(
    ptr,
    fmt::format(
      "Invalid allreduce state cast for id={:x} idx={} Scalar_typeid={} "
      "DataT_typeid={}\n",
      idx, states.size(), typeid(Scalar).name(), typeid(DataT).name()));
  return *ptr;
}

template <
  typename ReduceT, typename DataT,
  typename Scalar = typename DataHandler<DataT>::Scalar, typename ProxyT,
  typename MapT>
static auto& getStateImpl(ProxyT proxy, MapT& states_map, size_t idx) {
  auto& [_, states] = states_map[proxy.get()];
  auto const num_states = states.size();

  if (idx >= num_states || num_states == 0) {
    states.resize(idx + 1);
  }

  if constexpr (std::is_same_v<ReduceT, RabenseifnerT>) {
    return getState<RabenseifnerState<Scalar, DataT>, DataT, Scalar>(
      states, idx, proxy.get());
  } else {
    return getState<RecursiveDoublingState<DataT>, DataT, Scalar>(
      states, idx, proxy.get());
  }
}

template <typename ReducerT, typename DataT, typename Scalar>
decltype(auto) StateHolder::getState(detail::StrongVrtProxy proxy, size_t idx) {
  return getStateImpl<ReducerT, DataT>(proxy, active_coll_states_, idx);
}

template <typename ReducerT, typename DataT, typename Scalar>
decltype(auto) StateHolder::getState(detail::StrongObjGroup proxy, size_t idx) {
  return getStateImpl<ReducerT, DataT>(proxy, active_obj_states_, idx);
}

template <typename ReducerT, typename DataT, typename Scalar>
decltype(auto) StateHolder::getState(detail::StrongGroup proxy, size_t idx) {
  return getStateImpl<ReducerT, DataT>(proxy, active_grp_states_, idx);
}

} // namespace vt::collective::reduce::allreduce

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_STATE_HOLDER_IMPL_H*/
