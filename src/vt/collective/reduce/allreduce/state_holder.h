/*
//@HEADER
// *****************************************************************************
//
//                                state_holder.h
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
    size_t id = 0;
    auto& allreducers = active_coll_states_[proxy.get()];

    if (not allreducers.empty()) {
      // Last element is invalidated (allreduce completed) or not completed
      // Generate new ID
      if (not allreducers.back() or allreducers.back()->active_) {
        id = allreducers.size();
      }
      // Most recent state is not active, don't generate new ID
      else if (not allreducers.back()->active_) {
        id = allreducers.size() - 1;
      }
    }

    return id;
  }

  template <typename ReducerT>
  static size_t getNextID(detail::StrongObjGroup proxy) {
    size_t id = 0;
    auto& allreducers = active_obj_states_[proxy.get()];

    if (not allreducers.empty()) {
      // Last element is invalidated (allreduce completed) or not completed
      // Generate new ID
      if (not allreducers.back() or allreducers.back()->active_) {
        id = allreducers.size();
      }
      // Most recent state is not active, don't generate new ID
      else if (not allreducers.back()->active_) {
        id = allreducers.size() - 1;
      }
    }

    return id;
  }

  static size_t getNextID(detail::StrongGroup group) {
    size_t id = 0;
    auto& allreducers = active_grp_states_[group.get()];

    if (not allreducers.empty()) {
      // Last element is invalidated (allreduce completed) or not completed
      // Generate new ID
      if (not allreducers.back() or allreducers.back()->active_) {
        id = allreducers.size();
      }
      // Most recent state is not active, don't generate new ID
      else if (not allreducers.back()->active_) {
        id = allreducers.size() - 1;
      }
    }

    return id;
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
      num_states > idx,
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
    typename ReduceT, typename DataT,
    typename Scalar = typename DataHandler<DataT>::Scalar, typename ProxyT,
    typename MapT>
  static auto& getStateImpl(ProxyT proxy, MapT& states_map, size_t idx) {
    auto& states = states_map[proxy.get()];
    auto const num_states = states.size();

    vtAssert(
      num_states >= idx,
      fmt::format(
        "Attempting to access state {} with total number of states {}!", idx,
        num_states));

    if (idx >= num_states || num_states == 0) {
      if constexpr (std::is_same_v<ReduceT, RabenseifnerT>) {
        states.push_back(std::make_unique<RabenseifnerState<Scalar, DataT>>());
      } else {
        states.push_back(std::make_unique<RecursiveDoublingState<DataT>>());
      }
    }

    vtAssert(
      states.at(idx),
      fmt::format("Attempting to access invalidated state at idx={}!", idx));

    if constexpr (std::is_same_v<ReduceT, RabenseifnerT>) {
      auto* ptr =
        dynamic_cast<RabenseifnerState<Scalar, DataT>*>(states.at(idx).get());
      vtAssert(
        ptr,
        fmt::format(
          "Invalid Rabenseifner cast at idx={} with size={}!", idx,
          states.size()));
      return *ptr;
    } else {
      auto* ptr =
        dynamic_cast<RecursiveDoublingState<DataT>*>(states.at(idx).get());
      vtAssert(
        ptr,
        fmt::format(
          "Invalid RecursiveDoubling cast at idx={} with size={}!", idx,
          states.size()));
      return *ptr;
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
static inline auto& getState(ComponentInfo info, size_t id) {
  if (info.first == ComponentT::VrtColl) {
    return StateHolder::getState<ReducerT, DataT>(
      detail::StrongVrtProxy{info.second}, id);
  } else if (info.first == ComponentT::ObjGroup) {
    return StateHolder::getState<ReducerT, DataT>(
      detail::StrongObjGroup{info.second}, id);
  } else {
    return StateHolder::getState<ReducerT, DataT>(
      detail::StrongGroup{info.second}, id);
  }
}

static inline void cleanupState(ComponentInfo info, size_t id) {
  if (info.first == ComponentT::VrtColl) {
    StateHolder::clearSingle(detail::StrongVrtProxy{info.second}, id);
  } else if (info.first == ComponentT::ObjGroup) {
    StateHolder::clearSingle(detail::StrongObjGroup{info.second}, id);
  } else {
    StateHolder::clearSingle(detail::StrongGroup{info.second}, id);
  }
}

} // namespace vt::collective::reduce::allreduce

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_STATE_HOLDER_H*/
