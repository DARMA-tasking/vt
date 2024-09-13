/*
//@HEADER
// *****************************************************************************
//
//                                state_holder.h
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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_STATE_HOLDER_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_STATE_HOLDER_H

#include "vt/collective/reduce/allreduce/data_handler.h"
#include "vt/collective/reduce/allreduce/helpers.h"
#include "vt/collective/reduce/allreduce/type.h"
#include "vt/collective/reduce/scoping/strong_types.h"
#include "vt/configs/types/types_type.h"
#include "vt/configs/debug/debug_print.h"
#include "vt/collective/reduce/allreduce/state.h"

#include <memory>
#include <type_traits>
#include <unordered_map>

namespace vt::collective::reduce::allreduce {

struct StateHolder {
  template <
    typename ReducerT, typename DataT,
    typename Scalar = typename DataHandler<DataT>::Scalar>
  static auto& getState(detail::StrongVrtProxy proxy, size_t idx) {
    return getStateImpl<ReducerT, DataT>(proxy, active_coll_states_, idx);
  }

  template <
    typename ReducerT, typename DataT,
    typename Scalar = typename DataHandler<DataT>::Scalar>
  static auto& getState(detail::StrongObjGroup proxy, size_t idx) {
    return getStateImpl<ReducerT, DataT>(proxy, active_obj_states_, idx);
  }

  template <
    typename ReducerT, typename DataT,
    typename Scalar = typename DataHandler<DataT>::Scalar>
  static auto& getState(detail::StrongGroup proxy, size_t idx) {
    return getStateImpl<ReducerT, DataT>(proxy, active_grp_states_, idx);
  }

  template <typename ReducerT>
  static size_t getNextID(detail::StrongVrtProxy proxy) {
    return active_coll_states_[proxy.get()].size();
  }

  template <typename ReducerT>
  static size_t getNextID(detail::StrongObjGroup proxy) {
    return active_obj_states_[proxy.get()].size();
  }

  static size_t getNextID(detail::StrongGroup group) {
    return active_grp_states_[group.get()].size();
  }

  static void clearSingle(detail::StrongVrtProxy proxy, size_t idx) {
    clearSingleImpl(proxy, active_coll_states_, idx);
  }

  static void clearSingle(detail::StrongObjGroup proxy, size_t idx) {
    clearSingleImpl(proxy, active_obj_states_, idx);
  }

  static void clearSingle(detail::StrongGroup group, size_t idx) {
    clearSingleImpl(group, active_grp_states_, idx);
  }

  static void clearAll(detail::StrongVrtProxy proxy) {
    // fmt::print("Clearing all states for VrtProxy={:x}\n", proxy.get());
    clearAllImpl(proxy, active_coll_states_);
  }

  static void clearAll(detail::StrongObjGroup proxy) {
    // fmt::print("Clearing all states for Objgroup={:x}\n", proxy.get());
    clearAllImpl(proxy, active_obj_states_);
  }

  static void clearAll(detail::StrongGroup group) {
    // fmt::print("Clearing all states for group={:x}\n", group.get());
    clearAllImpl(group, active_grp_states_);
  }

private:
  template <typename ProxyT, typename MapT>
  static void clearSingleImpl(ProxyT proxy, MapT& states_map, size_t idx) {
    auto& states = states_map[proxy.get()];

    auto const num_states = states.size();
    vtAssert(
      num_states >= idx,
      fmt::format(
        "Attempting to access state {} with total numer of states {}!", idx,
        num_states));

    states.at(idx).reset();
  }

  template <typename ProxyT, typename MapT>
  static void clearAllImpl(ProxyT proxy, MapT& states_map) {
    states_map.erase(proxy.get());
  }

  template <
    typename ReduceT, typename DataT, typename ProxyT, typename MapT,
    typename Scalar = typename DataHandler<DataT>::Scalar>
  static auto& getStateImpl(ProxyT proxy, MapT& states_map, size_t idx) {
    auto& states = states_map[proxy.get()];

    auto const num_states = states.size();
    vtAssert(
      num_states >= idx,
      fmt::format(
        "Attempting to access state {} with total numer of states {}!", idx,
        num_states));

    if (idx >= num_states or (num_states == 0)) {
      if constexpr (std::is_same_v<ReduceT, RabenseifnerT>) {
        states.push_back(std::make_unique<RabenseifnerState<Scalar, DataT>>());
      } else {
        states.push_back(
          std::make_unique<RecursiveDoublingState<DataT>>());
      }
    }

    vtAssert(
      states.at(idx),
      fmt::format("Attempting to access invalidated state at idx={}!", idx));
    if constexpr (std::is_same_v<ReduceT, RabenseifnerT>) {
      return dynamic_cast<RabenseifnerState<Scalar, DataT>&>(*(states.at(idx)));
    } else {
      return dynamic_cast<RecursiveDoublingState<DataT>&>(
        *(states.at(idx)));
    }
  }

  static inline std::unordered_map<
    VirtualProxyType, std::vector<std::unique_ptr<StateBase>>>
    active_coll_states_ = {};

  static inline std::unordered_map<
    ObjGroupProxyType, std::vector<std::unique_ptr<StateBase>>>
    active_obj_states_ = {};

  static inline std::unordered_map<
    GroupType, std::vector<std::unique_ptr<StateBase>>>
    active_grp_states_ = {};
};

template <typename ReducerT, typename DataT>
static inline auto&
getState(VirtualProxyType coll, ObjGroupProxyType obj, GroupType g, size_t id) {
  if (coll != u64empty) {
    return StateHolder::getState<ReducerT, DataT>(
      detail::StrongVrtProxy{coll}, id);
  } else if (obj != u64empty) {
    return StateHolder::getState<ReducerT, DataT>(
      detail::StrongObjGroup{obj}, id);
  } else {
    return StateHolder::getState<ReducerT, DataT>(detail::StrongGroup{g}, id);
  }
}

static inline void cleanupState(
  VirtualProxyType coll, ObjGroupProxyType obj, GroupType g, size_t id) {
  if (coll != u64empty) {
    StateHolder::clearSingle(detail::StrongVrtProxy{coll}, id);
  } else if (obj != u64empty) {
    StateHolder::clearSingle(detail::StrongObjGroup{obj}, id);
  } else {
    StateHolder::clearSingle(detail::StrongGroup{g}, id);
  }
}

} // namespace vt::collective::reduce::allreduce

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_STATE_HOLDER_H*/
