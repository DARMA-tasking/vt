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
#include "vt/collective/reduce/allreduce/type.h"
#include "vt/collective/reduce/scoping/strong_types.h"
#include "vt/configs/types/types_type.h"
#include "vt/collective/reduce/allreduce/state.h"

#include <memory>
#include <unordered_map>

namespace vt::collective::reduce::allreduce {

struct StateHolder {
  using StatesVec = std::vector<std::unique_ptr<StateBase>>;
  using StatesInfo = std::pair<size_t, StatesVec>;

  template <
    typename ReducerT, typename DataT,
    typename Scalar = typename DataHandler<DataT>::Scalar>
  static decltype(auto) getState(detail::StrongVrtProxy proxy, size_t idx);

  template <
    typename ReducerT, typename DataT,
    typename Scalar = typename DataHandler<DataT>::Scalar>
  static decltype(auto) getState(detail::StrongObjGroup proxy, size_t idx);

  template <
    typename ReducerT, typename DataT,
    typename Scalar = typename DataHandler<DataT>::Scalar>
  static decltype(auto) getState(detail::StrongGroup proxy, size_t idx);

  static size_t getNextID(detail::StrongVrtProxy proxy);
  static size_t getNextID(detail::StrongObjGroup proxy);
  static size_t getNextID(detail::StrongGroup group);

  static void clearSingle(detail::StrongVrtProxy proxy, size_t idx);
  static void clearSingle(detail::StrongObjGroup proxy, size_t idx);
  static void clearSingle(detail::StrongGroup group, size_t idx);

  static void clearAll(detail::StrongVrtProxy proxy);
  static void clearAll(detail::StrongObjGroup proxy);
  static void clearAll(detail::StrongGroup group);

private:
  static inline std::unordered_map<VirtualProxyType, StatesInfo>
    active_coll_states_ = {};

  static inline std::unordered_map<ObjGroupProxyType, StatesInfo>
    active_obj_states_ = {};

  static inline std::unordered_map<GroupType, StatesInfo> active_grp_states_ =
    {};
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

#include "state_holder.impl.h"

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_STATE_HOLDER_H*/
