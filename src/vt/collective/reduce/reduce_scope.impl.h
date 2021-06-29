/*
//@HEADER
// *****************************************************************************
//
//                             reduce_scope.impl.h
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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_REDUCE_SCOPE_IMPL_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_REDUCE_SCOPE_IMPL_H

#include "vt/config.h"

namespace vt { namespace collective { namespace reduce { namespace detail {

template <typename T, typename... Args>
inline ReduceScope makeScope(Args&&... args) {
  ReduceScope scope;
  scope.l0_.init<T>(std::forward<Args>(args)...);
  return scope;
}

inline std::string stringizeStamp(ReduceStamp const& stamp) {
  if (stamp.is<StrongTag>()) {
    return fmt::format("tag[{}]", stamp.get<StrongTag>().get());
  } else if (stamp.is<TagPair>()) {
    return fmt::format(
      "tagPair[{},{}]",
      stamp.get<TagPair>().first(),
      stamp.get<TagPair>().second()
    );
  } else if (stamp.is<StrongSeq>()) {
    return fmt::format("seq[{}]", stamp.get<StrongSeq>().get());
  } else if (stamp.is<StrongUserID>()) {
    return fmt::format("userID[{}]", stamp.get<StrongUserID>().get());
  } else if (stamp.is<StrongEpoch>()) {
    return fmt::format("epoch[{:x}]", stamp.get<StrongEpoch>().get());
  } else {
    return "<no-stamp>";
  }
}

template <typename T>
T& ReduceScopeHolder<T>::get(ReduceScope const& scope) {
  vt_debug_print(
    verbose, reduce,
    "ReduceScopeHolder get scope={}\n",
    scope.str()
  );

  auto iter = scopes_.find(scope);
  vtAssert(iter != scopes_.end(), "Scope must exist");
  return iter->second;
}

template <typename T>
template <typename U>
T& ReduceScopeHolder<T>::getOnDemand(U&& scope) {
  auto iter = scopes_.find(scope);
  if (iter == scopes_.end()) {
    vtAssert(
      not scope.get().template is<StrongGroup>(),
      "Group reducers cannot be on-demand created -- needs spanning tree"
    );

    iter = scopes_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(scope),
      std::forward_as_tuple(default_creator_(scope))
    ).first;
  }

  vtAssert(iter != scopes_.end(), "Scope must exist");

  return iter->second;
}

template <typename T>
T& ReduceScopeHolder<T>::get(ObjGroupTag, ObjGroupProxyType proxy) {
  return getOnDemand(makeScope<StrongObjGroup>(proxy));
}

template <typename T>
T& ReduceScopeHolder<T>::get(VrtProxyTag, VirtualProxyType proxy) {
  return getOnDemand(makeScope<StrongVrtProxy>(proxy));
}

template <typename T>
T& ReduceScopeHolder<T>::get(GroupTag, GroupType group) {
  return get(makeScope<StrongGroup>(group));
}

template <typename T>
T& ReduceScopeHolder<T>::get(ComponentTag, ComponentIDType component_id) {
  return getOnDemand(makeScope<StrongCom>(component_id));
}

template <typename T>
T& ReduceScopeHolder<T>::get(UserIDTag, UserIDType user_id) {
  return getOnDemand(makeScope<StrongUserID>(user_id));
}

template <typename T>
void ReduceScopeHolder<T>::make(ObjGroupTag, ObjGroupProxyType proxy) {
  getOnDemand(makeScope<StrongObjGroup>(proxy));
}

template <typename T>
void ReduceScopeHolder<T>::make(
  GroupTag, GroupType group, DefaultCreateFunction fn
) {
  ReduceScope scope = makeScope<StrongGroup>(group);
  scopes_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(scope),
    std::forward_as_tuple(fn(scope))
  );
}


}}}} /* end namespace vt::collective::reduce::detail */

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_REDUCE_SCOPE_IMPL_H*/
