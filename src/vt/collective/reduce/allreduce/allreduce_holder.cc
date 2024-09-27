/*
//@HEADER
// *****************************************************************************
//
//                             allreduce_holder.cc
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
#include "allreduce_holder.h"
#include "vt/objgroup/manager.h"

namespace vt::collective::reduce::allreduce {

void AllreduceHolder::createAllreducers(detail::StrongGroup strong_group) {
  addRabensifnerAllreducer(strong_group);
  addRecursiveDoublingAllreducer(strong_group);
}

Rabenseifner* AllreduceHolder::addRabensifnerAllreducer(
  detail::StrongVrtProxy strong_proxy, detail::StrongGroup strong_group,
  size_t num_elems) {
  auto const coll_proxy = strong_proxy.get();

  auto obj_proxy = new Rabenseifner(strong_proxy, strong_group, num_elems);

  col_reducers_[coll_proxy].first = obj_proxy;

  vt_debug_print(
    verbose, allreduce, "Adding new Rabenseifner reducer for collection={:x}",
    coll_proxy
  );

  return obj_proxy;
}

RecursiveDoubling*
AllreduceHolder::addRecursiveDoublingAllreducer(
  detail::StrongVrtProxy strong_proxy, detail::StrongGroup strong_group,
  size_t num_elems) {
  auto const coll_proxy = strong_proxy.get();
  auto obj_proxy = new RecursiveDoubling(
    strong_proxy, strong_group, num_elems);

  col_reducers_[coll_proxy].second = obj_proxy;

  vt_debug_print(
    verbose, allreduce,
    "Adding new RecursiveDoubling reducer for collection={:x}", coll_proxy
  );

  return obj_proxy;
}

Rabenseifner*
AllreduceHolder::addRabensifnerAllreducer(detail::StrongGroup strong_group) {
  auto const group = strong_group.get();

  auto obj_proxy = new Rabenseifner(
    strong_group);

  group_reducers_[group].first = obj_proxy;

  vt_debug_print(
    verbose, allreduce,
    "Adding new Rabenseifner reducer for group={:x}", group
  );

  return obj_proxy;
}

RecursiveDoubling*
AllreduceHolder::addRecursiveDoublingAllreducer(
  detail::StrongGroup strong_group) {
  auto const group = strong_group.get();

  auto obj_proxy = new RecursiveDoubling(
    strong_group);

  vt_debug_print(
    verbose, allreduce,
    "Adding new Rabenseifner reducer for group={:x}", group
  );

  group_reducers_[group].second = obj_proxy;

  return obj_proxy;
}

Rabenseifner*
AllreduceHolder::addRabensifnerAllreducer(detail::StrongObjGroup strong_objgroup) {
  auto const objgroup = strong_objgroup.get();

  auto obj_proxy = new Rabenseifner(
    strong_objgroup);

  objgroup_reducers_[objgroup].first = obj_proxy;

  vt_debug_print(
    verbose, allreduce,
    "Adding new Rabenseifner reducer for objgroup={:x} Size={}\n", objgroup
  );

  return obj_proxy;
}

RecursiveDoubling*
AllreduceHolder::addRecursiveDoublingAllreducer(
  detail::StrongObjGroup strong_objgroup) {
  auto const objgroup = strong_objgroup.get();

  auto obj_proxy = new RecursiveDoubling(
    strong_objgroup);

  vt_debug_print(
    verbose, allreduce,
    "Adding new RecursiveDoubling reducer for objgroup={:x}\n", objgroup
  );

  objgroup_reducers_[objgroup].second = obj_proxy;

  return obj_proxy;
}

void AllreduceHolder::remove(detail::StrongVrtProxy strong_proxy) {
  auto const key = strong_proxy.get();

  auto it = col_reducers_.find(key);

  if (it != col_reducers_.end()) {
    col_reducers_.erase(key);
  }
}

void AllreduceHolder::remove(detail::StrongGroup strong_group) {
  auto const key = strong_group.get();

  auto it = group_reducers_.find(key);

  if (it != group_reducers_.end()) {
    group_reducers_.erase(key);
  }
}

void AllreduceHolder::remove(detail::StrongObjGroup strong_objgroup) {
  auto const key = strong_objgroup.get();

  auto it = objgroup_reducers_.find(key);

  if (it != objgroup_reducers_.end()) {
    objgroup_reducers_.erase(key);
  }
}

} // namespace vt::collective::reduce::allreduce
