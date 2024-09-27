/*
//@HEADER
// *****************************************************************************
//
//                              allreduce_holder.h
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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_ALLREDUCE_HOLDER_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_ALLREDUCE_HOLDER_H

#include "vt/configs/types/types_type.h"
#include "vt/collective/reduce/allreduce/type.h"
#include "vt/collective/reduce/scoping/strong_types.h"
#include "vt/configs/types/types_sentinels.h"
#include "vt/objgroup/proxy/proxy_objgroup.h"

#include <type_traits>
#include <unordered_map>

namespace vt::collective::reduce::allreduce {

struct Rabenseifner;
struct RecursiveDoubling;
struct AllreduceHolder {
  using RabenseifnerProxy = ObjGroupProxyType;
  using RecursiveDoublingProxy = ObjGroupProxyType;

  static void createAllreducers(detail::StrongGroup strong_group);

  template <typename ReducerT>
  static auto getAllreducer(
    detail::StrongVrtProxy strong_proxy) {
    auto const coll_proxy = strong_proxy.get();

    auto it = col_reducers_.find(coll_proxy);
    if(it == col_reducers_.end()){
      col_reducers_[coll_proxy] = {nullptr, nullptr};
    }

    if constexpr(std::is_same_v<ReducerT, RabenseifnerT>){
      return col_reducers_.at(coll_proxy).first;
    }else {
      return col_reducers_.at(coll_proxy).second;
    }
  }

  template <typename ReducerT>
  static auto getAllreducer(
    detail::StrongGroup strong_group) {
    auto const group = strong_group.get();

    auto it = group_reducers_.find(group);
    if(it == group_reducers_.end()){
      group_reducers_[group] = {nullptr, nullptr};
    }

    if constexpr(std::is_same_v<ReducerT, RabenseifnerT>){
      return group_reducers_.at(group).first;
    }else {
      return group_reducers_.at(group).second;
    }
  }

  template <typename ReducerT>
  static auto getAllreducer(
    detail::StrongObjGroup strong_objgroup) {
    auto const objgroup = strong_objgroup.get();

    auto it = objgroup_reducers_.find(objgroup);
    if(it == objgroup_reducers_.end()){
      objgroup_reducers_[objgroup] = {nullptr, nullptr};
    }

    if constexpr(std::is_same_v<ReducerT, RabenseifnerT>){
      return objgroup_reducers_.at(objgroup).first;
    }else {
      return objgroup_reducers_.at(objgroup).second;
    }
  }

  template <typename ReducerT>
  static auto getOrCreateAllreducer(
    detail::StrongVrtProxy strong_proxy, detail::StrongGroup strong_group,
    size_t num_elems) {
    auto const coll_proxy = strong_proxy.get();

    if (col_reducers_.find(coll_proxy) == col_reducers_.end()) {
      col_reducers_[coll_proxy] = {nullptr, nullptr};
    }

    if constexpr (std::is_same_v<ReducerT, RabenseifnerT>) {
      auto reducer = col_reducers_.at(coll_proxy).first;
      if (reducer == nullptr) {
        return addRabensifnerAllreducer(strong_proxy, strong_group, num_elems);
      } else {
        return reducer;
      }
    } else {
      auto reducer = col_reducers_.at(coll_proxy).second;
      if (reducer == nullptr) {
        return addRecursiveDoublingAllreducer(
          strong_proxy, strong_group, num_elems);
      } else {
        return reducer;
      }
    }
  }

  template <typename ReducerT>
  static auto getOrCreateAllreducer(detail::StrongGroup strong_group) {
    auto const group = strong_group.get();

    if (auto it = group_reducers_.find(group); it == group_reducers_.end()) {
      group_reducers_[group] = {nullptr, nullptr};
    }

    if constexpr (std::is_same_v<ReducerT, RabenseifnerT>) {
      auto reducer = group_reducers_.at(group).first;
      if (reducer == nullptr) {
        return addRabensifnerAllreducer(strong_group);
      } else {
        return reducer;
      }
    } else {
      auto reducer = group_reducers_.at(group).second;
      if (reducer == nullptr) {
        return addRecursiveDoublingAllreducer(strong_group);
      } else {
        return reducer;
      }
    }
  }

  template <typename ReducerT>
  static auto getOrCreateAllreducer(detail::StrongObjGroup strong_objgroup) {
    auto const objgroup = strong_objgroup.get();

    if (auto it = objgroup_reducers_.find(objgroup); it == objgroup_reducers_.end()) {
      objgroup_reducers_[objgroup] = {nullptr, nullptr};
    }

    if constexpr (std::is_same_v<ReducerT, RabenseifnerT>) {
      auto reducer = objgroup_reducers_.at(objgroup).first;
      if (reducer == nullptr) {
        return addRabensifnerAllreducer(strong_objgroup);
      } else {
        return reducer;
      }
    } else {
      auto reducer = objgroup_reducers_.at(objgroup).second;
      if (reducer == nullptr) {
        return addRecursiveDoublingAllreducer(strong_objgroup);
      } else {
        return reducer;
      }
    }
  }

  static Rabenseifner* addRabensifnerAllreducer(
    detail::StrongVrtProxy strong_proxy, detail::StrongGroup strong_group,
    size_t num_elems);

  static RecursiveDoubling*
  addRecursiveDoublingAllreducer(
    detail::StrongVrtProxy strong_proxy, detail::StrongGroup strong_group,
    size_t num_elems);

  static Rabenseifner*
  addRabensifnerAllreducer(detail::StrongGroup strong_group);
  static RecursiveDoubling*
  addRecursiveDoublingAllreducer(detail::StrongGroup strong_group);

  static Rabenseifner*
  addRabensifnerAllreducer(detail::StrongObjGroup strong_group);
  static RecursiveDoubling*
  addRecursiveDoublingAllreducer(detail::StrongObjGroup strong_group);

  static void remove(detail::StrongVrtProxy strong_proxy);
  static void remove(detail::StrongGroup strong_group);
  static void remove(detail::StrongObjGroup strong_group);

  static inline std::unordered_map<
    VirtualProxyType, std::pair<Rabenseifner*, RecursiveDoubling*>>
    col_reducers_ = {};
  static inline std::unordered_map<
    GroupType, std::pair<Rabenseifner*, RecursiveDoubling*>>
    group_reducers_ = {};
  static inline std::unordered_map<
    ObjGroupProxyType, std::pair<Rabenseifner*, RecursiveDoubling*>>
    objgroup_reducers_ = {};
};

template <typename ReducerT>
static inline auto* getAllreducer(ComponentInfo type) {
  if (type.first == ComponentT::VrtColl) {
    return AllreduceHolder::getAllreducer<ReducerT>(
      detail::StrongVrtProxy{type.second});
  } else if (type.first == ComponentT::ObjGroup) {
    return AllreduceHolder::getAllreducer<ReducerT>(
      detail::StrongObjGroup{type.second});
  } else {
    return AllreduceHolder::getAllreducer<ReducerT>(
      detail::StrongGroup{type.second});
  }
}

} // namespace vt::collective::reduce::allreduce

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_ALLREDUCE_HOLDER_H*/
