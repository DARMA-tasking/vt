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

#include <unordered_map>

namespace vt::collective::reduce::allreduce {

struct Rabenseifner;
struct RecursiveDoubling;
struct AllreduceHolder {
  using RabenseifnerProxy = ObjGroupProxyType;
  using RecursiveDoublingProxy = ObjGroupProxyType;

  template <typename ReducerT>
  static decltype(auto) getAllreducer(detail::StrongVrtProxy strong_proxy);

  template <typename ReducerT>
  static decltype(auto) getAllreducer(detail::StrongGroup strong_group);

  template <typename ReducerT>
  static decltype(auto) getAllreducer(detail::StrongObjGroup strong_objgroup);

  template <typename ReducerT>
  static decltype(auto) getOrCreateAllreducer(
    detail::StrongVrtProxy strong_proxy, detail::StrongGroup strong_group,
    size_t num_elems);

  template <typename ReducerT>
  static decltype(auto) getOrCreateAllreducer(detail::StrongGroup strong_group);

  template <typename ReducerT>
  static decltype(auto)
  getOrCreateAllreducer(detail::StrongObjGroup strong_objgroup);

  static void remove(detail::StrongVrtProxy strong_proxy);
  static void remove(detail::StrongGroup strong_group);
  static void remove(detail::StrongObjGroup strong_group);

private:
  template <typename ReducerT, typename MapT>
  static decltype(auto) getAllreducerImpl(MapT& map, uint64_t id);

  template <typename ReducerT, typename MapT, typename... Args>
  static decltype(auto) getOrCreateAllreducerImpl(MapT& map, uint64_t id, Args&&... args);

  static Rabenseifner* addRabensifnerAllreducer(
    detail::StrongVrtProxy strong_proxy, detail::StrongGroup strong_group,
    size_t num_elems);

  static RecursiveDoubling* addRecursiveDoublingAllreducer(
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

#include "vt/collective/reduce/allreduce/allreduce_holder.impl.h"

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_ALLREDUCE_HOLDER_H*/
