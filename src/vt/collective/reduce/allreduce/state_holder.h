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

#include <memory>
#include <type_traits>
#include <unordered_map>

namespace vt::collective::reduce::allreduce {

struct StateHolder {
  template <
    typename ReducerT, typename DataT,
    typename Scalar = typename DataHandler<DataT>::Scalar>
  static State<Scalar, DataT>&
  getState(detail::StrongVrtProxy proxy, size_t idx) {
    // vt_debug_print(
    //   terse, allreduce,
    //   "Vrt get state for proxy = {} idx = {} active_coll_states_.size() = {}\n",
    //   proxy.get(), idx, active_coll_states_.size());
    return getStateImpl<DataT>(proxy, active_coll_states_, idx);
    // if constexpr (std::is_same_v<ReducerT, RabenseifnerT>) {
    //   return getStateImpl<DataT>(proxy, active_coll_rabenseifner_states_, idx);
    // } else {
    //   return getStateImpl<DataT>(
    //     proxy, active_coll_recursive_doubling_states_, idx);
    // }
  }

  template <
    typename ReducerT, typename DataT,
    typename Scalar = typename DataHandler<DataT>::Scalar>
  static State<Scalar, DataT>&
  getState(detail::StrongObjGroup proxy, size_t idx) {
    if constexpr (std::is_same_v<ReducerT, RabenseifnerT>) {
      return getStateImpl<DataT>(proxy, active_obj_rabenseifner_states_, idx);
    } else {
      return getStateImpl<DataT>(
        proxy, active_obj_recursive_doubling_states_, idx);
    }
  }

  template <
    typename ReducerT, typename DataT,
    typename Scalar = typename DataHandler<DataT>::Scalar>
  static State<Scalar, DataT>& getState(detail::StrongGroup proxy, size_t idx) {
    if constexpr (std::is_same_v<ReducerT, RabenseifnerT>) {
      return getStateImpl<DataT>(proxy, active_grp_rabenseifner_states_, idx);
    } else {
      return getStateImpl<DataT>(
        proxy, active_grp_recursive_doubling_states_, idx);
    }
  }

  template <typename ReducerT>
  static size_t getNextID(detail::StrongVrtProxy proxy) {
    return active_coll_states_[proxy.get()].size();
    // if constexpr (std::is_same_v<ReducerT, RabenseifnerT>) {

    // } else {
    //   return active_coll_recursive_doubling_states_[proxy.get()].size();
    // }
  }

  template <typename ReducerT>
  static size_t getNextID(detail::StrongObjGroup proxy) {
    if constexpr (std::is_same_v<ReducerT, RabenseifnerT>) {
      return active_obj_rabenseifner_states_[proxy.get()].size();
    } else {
      return active_obj_recursive_doubling_states_[proxy.get()].size();
    }
  }

  template <typename ReducerT>
  static size_t getNextID(detail::StrongGroup group) {
    if constexpr (std::is_same_v<ReducerT, RabenseifnerT>) {
      return active_grp_rabenseifner_states_[group.get()].size();
    } else {
      return active_grp_recursive_doubling_states_[group.get()].size();
    }
  }

  template <typename ReducerT>
  static void clear(detail::StrongVrtProxy proxy, size_t idx) {
    clearImpl(proxy, active_coll_states_, idx);
    // if constexpr (std::is_same_v<ReducerT, RabenseifnerT>) {
    //   clearImpl(proxy, active_coll_states_, idx);
    // } else {
    //   clearImpl(proxy, active_coll_recursive_doubling_states_, idx);
    // }
  }

  template <typename ReducerT>
  static void clear(detail::StrongObjGroup proxy, size_t idx) {
    if constexpr (std::is_same_v<ReducerT, RabenseifnerT>) {
      clearImpl(proxy, active_obj_rabenseifner_states_, idx);
    } else {
      clearImpl(proxy, active_obj_recursive_doubling_states_, idx);
    }
  }

  template <typename ReducerT>
  static void clear(detail::StrongGroup group, size_t idx) {
    if constexpr (std::is_same_v<ReducerT, RabenseifnerT>) {
      clearImpl(group, active_grp_rabenseifner_states_, idx);
    } else {
      clearImpl(group, active_grp_recursive_doubling_states_, idx);
    }
  }

private:
  template <typename ProxyT, typename MapT>
  static void clearImpl(ProxyT proxy, MapT& states_map, size_t idx) {
    auto& states = states_map[proxy.get()];

    auto const num_states = states.size();
    vtAssert(
      num_states >= idx,
      fmt::format(
        "Attempting to access state {} with total numer of states {}!", idx,
        num_states));

    states.at(idx).reset();
  }

  template <
    typename DataT, typename ProxyT, typename MapT,
    typename Scalar = typename DataHandler<DataT>::Scalar>
  static State<Scalar, DataT>&
  getStateImpl(ProxyT proxy, MapT& states_map, size_t idx) {
    auto& states = states_map[proxy.get()];

    auto const num_states = states.size();
    vtAssert(
      num_states >= idx,
      fmt::format(
        "Attempting to access state {} with total numer of states {}!", idx,
        num_states));

    if (idx >= num_states or (num_states == 0)) {
      states.push_back(std::make_unique<State<Scalar, DataT>>());
    }
    return dynamic_cast<State<Scalar, DataT>&>(*(states.at(idx)));
  }

  static inline std::unordered_map<
    VirtualProxyType, std::vector<std::unique_ptr<StateBase>>>
    active_coll_states_ = {};

  /// VrtColl holders
  // static inline std::unordered_map<
  //   VirtualProxyType, std::vector<std::unique_ptr<StateBase>>>
  //   active_coll_rabenseifner_states_ = {};

  // static inline std::unordered_map<
  //   VirtualProxyType, std::vector<std::unique_ptr<StateBase>>>
  //   active_coll_recursive_doubling_states_ = {};

  /// ObjGRoup holders
  static inline std::unordered_map<
    ObjGroupProxyType, std::vector<std::unique_ptr<StateBase>>>
    active_obj_rabenseifner_states_ = {};

  static inline std::unordered_map<
    ObjGroupProxyType, std::vector<std::unique_ptr<StateBase>>>
    active_obj_recursive_doubling_states_ = {};

  /// Group holders
  static inline std::unordered_map<
    GroupType, std::vector<std::unique_ptr<StateBase>>>
    active_grp_rabenseifner_states_ = {};

  static inline std::unordered_map<
    GroupType, std::vector<std::unique_ptr<StateBase>>>
    active_grp_recursive_doubling_states_ = {};
};

} // namespace vt::collective::reduce::allreduce

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_STATE_HOLDER_H*/
