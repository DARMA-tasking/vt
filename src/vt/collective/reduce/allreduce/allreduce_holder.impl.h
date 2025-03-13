/*
//@HEADER
// *****************************************************************************
//
//                           allreduce_holder.impl.h
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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_ALLREDUCE_HOLDER_IMPL_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_ALLREDUCE_HOLDER_IMPL_H

#include "vt/collective/reduce/allreduce/allreduce_holder.h"

#include <type_traits>

namespace vt::collective::reduce::allreduce {

template <typename ReducerT, typename MapT>
decltype(auto) AllreduceHolder::getAllreducerImpl(MapT& map, uint64_t id) {
  auto it = map.find(id);
  if (it == map.end()) {
    map[id] = {nullptr, nullptr};
  }

  if constexpr (std::is_same_v<ReducerT, RabenseifnerT>) {
    return map.at(id).first;
  } else {
    return map.at(id).second;
  }
}

template <typename ReducerT>
decltype(auto)
AllreduceHolder::getAllreducer(detail::StrongVrtProxy strong_proxy) {
  auto const coll_proxy = strong_proxy.get();

  return getAllreducerImpl<ReducerT>(col_reducers_, coll_proxy);
}

template <typename ReducerT>
decltype(auto) AllreduceHolder::getAllreducer(detail::StrongGroup strong_group) {
  auto const group = strong_group.get();

  return getAllreducerImpl<ReducerT>(group_reducers_, group);
}

template <typename ReducerT>
decltype(auto)
AllreduceHolder::getAllreducer(detail::StrongObjGroup strong_objgroup) {
  auto const objgroup = strong_objgroup.get();

  return getAllreducerImpl<ReducerT>(objgroup_reducers_, objgroup);
}

template <typename ReducerT, typename MapT, typename... Args>
decltype(auto) AllreduceHolder::getOrCreateAllreducerImpl(
  MapT& map, uint64_t id, Args&&... args) {
  if (map.find(id) == col_reducers_.end()) {
    map[id] = {nullptr, nullptr};
  }

  if constexpr (std::is_same_v<ReducerT, RabenseifnerT>) {
    auto reducer = map.at(id).first;
    if (reducer == nullptr) {
      return addRabensifnerAllreducer(std::forward<Args>(args)...);
    } else {
      return reducer;
    }
  } else {
    auto reducer = map.at(id).second;
    if (reducer == nullptr) {
      return addRecursiveDoublingAllreducer(std::forward<Args>(args)...);
    } else {
      return reducer;
    }
  }
}

template <typename ReducerT>
decltype(auto) AllreduceHolder::getOrCreateAllreducer(
  detail::StrongVrtProxy strong_proxy, detail::StrongGroup strong_group,
  size_t num_elems) {
  auto const coll_proxy = strong_proxy.get();

  return getOrCreateAllreducerImpl<ReducerT>(
    col_reducers_, coll_proxy, strong_proxy, strong_group, num_elems);
}

template <typename ReducerT>
decltype(auto)
AllreduceHolder::getOrCreateAllreducer(detail::StrongGroup strong_group) {
  auto const group = strong_group.get();
  return getOrCreateAllreducerImpl<ReducerT>(
    col_reducers_, group, strong_group);
}

template <typename ReducerT>
decltype(auto)
AllreduceHolder::getOrCreateAllreducer(detail::StrongObjGroup strong_objgroup) {
  auto const objgroup = strong_objgroup.get();
  return getOrCreateAllreducerImpl<ReducerT>(
    objgroup_reducers_, objgroup, strong_objgroup);
}

} // namespace vt::collective::reduce::allreduce

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_ALLREDUCE_HOLDER_IMPL_H*/
