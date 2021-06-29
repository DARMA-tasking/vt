/*
//@HEADER
// *****************************************************************************
//
//                              reduce_manager.cc
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

#include "vt/collective/reduce/reduce_manager.h"
#include "vt/collective/reduce/reduce.h"

namespace vt { namespace collective { namespace reduce {

ReduceManager::ReduceManager()
  : reducers_( // default cons reducer for non-group
      [](detail::ReduceScope const& scope) {
        return std::make_unique<Reduce>(scope);
      }
    )
{
  // insert the default reducer scope
  reducers_.make(
    typename ReduceScopeType::GroupTag{}, default_group,
    [](detail::ReduceScope const& scope) -> std::unique_ptr<Reduce> {
      return std::make_unique<Reduce>(scope);
    }
  );
}

Reduce* ReduceManager::global() {
  return getReducerGroup(default_group);
}

Reduce* ReduceManager::getReducer(detail::ReduceScope const& scope) {
  return reducers_.getOnDemand(scope).get();
}

Reduce* ReduceManager::getReducerObjGroup(ObjGroupProxyType const& proxy) {
  return reducers_.get(typename ReduceScopeType::ObjGroupTag{}, proxy).get();
}

Reduce* ReduceManager::getReducerVrtProxy(VirtualProxyType const& proxy) {
  return reducers_.get(typename ReduceScopeType::VrtProxyTag{}, proxy).get();
}

Reduce* ReduceManager::getReducerGroup(GroupType const& group) {
  return reducers_.get(typename ReduceScopeType::GroupTag{}, group).get();
}

Reduce* ReduceManager::getReducerComponent(ComponentIDType const& cid) {
  return reducers_.get(typename ReduceScopeType::ComponentTag{}, cid).get();
}

Reduce* ReduceManager::makeReducerCollective() {
  return reducers_.get(typename ReduceScopeType::UserIDTag{}, cur_user_id_++).get();
}

void ReduceManager::makeReducerGroup(
  GroupType const& group, collective::tree::Tree* tree
) {
  reducers_.make(
    typename ReduceScopeType::GroupTag{}, group,
    [=](detail::ReduceScope const& scope) -> std::unique_ptr<Reduce> {
      return std::make_unique<Reduce>(scope, tree);
    }
  );
}

}}} /* end namespace vt::collective::reduce */
