/*
//@HEADER
// *****************************************************************************
//
//                             group_manager.impl.h
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

#include "vt/objgroup/proxy/proxy_objgroup.h"
#if !defined INCLUDED_VT_GROUP_GROUP_MANAGER_IMPL_H
#define INCLUDED_VT_GROUP_GROUP_MANAGER_IMPL_H

#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/group/group_common.h"
#include "vt/group/group_manager.h"
#include "vt/group/id/group_id.h"
#include "vt/group/region/group_region.h"
#include "vt/registry/auto/auto_registry_interface.h"
#include "vt/messaging/message.h"
#include "vt/messaging/active.h"
#include "vt/activefn/activefn.h"
#include "vt/group/group_info.h"
#include "vt/collective/reduce/allreduce/rabenseifner.h"
#include "vt/objgroup/manager.h"

namespace vt { namespace group {

template <typename T>
/*static*/ typename GroupManagerT<T>::ActionContainerTType
GroupManagerT<T>::continuation_actions_t_ = {};

template <typename T>
/*static*/ std::unordered_map<RemoteOperationIDType,std::vector<T>>
GroupManagerT<T>::waiting_cont_ = {};

template <typename T>
void GroupManagerT<T>::pushCleanupAction() {
  // Push the typeless cleanup actions
  if (continuation_actions_t_.size() == 0) {
    theGroup()->addCleanupAction([]{ continuation_actions_t_.clear(); });
  }
  if (waiting_cont_.size() == 0) {
    theGroup()->addCleanupAction([]{ waiting_cont_.clear(); });
  }
}

template <typename T>
RemoteOperationIDType GroupManagerT<T>::registerContinuationT(ActionTType act) {
  pushCleanupAction();

  RemoteOperationIDType next_id = theGroup()->getNextOpID();
  continuation_actions_t_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(next_id),
    std::forward_as_tuple(ActionListTType{act})
  );
  return next_id;
}

template <typename T>
void GroupManagerT<T>::registerContinuationT(
  RemoteOperationIDType const op, ActionTType action
) {
  vt_debug_print(
    verbose, group,
    "GroupManager::registerContinuationT: op={:x}\n", op
  );

  pushCleanupAction();

  continuation_actions_t_[op].push_back(action);
}

template <typename T>
/*static*/ void GroupManagerT<T>::triggerWaitingContinuations(
  RemoteOperationIDType const op
) {
  auto action_iter = continuation_actions_t_.find(op);
  auto const found = action_iter != continuation_actions_t_.end();

  if (found) {
    auto iter = waiting_cont_.find(op);
    if (iter != waiting_cont_.end()) {
      for (auto&& elm : iter->second) {
        for (auto&& action : action_iter->second) {
          action(elm);
        }
      }
      waiting_cont_.clear();
    }
  }
}

template <typename T>
void GroupManagerT<T>::triggerContinuationT(
  RemoteOperationIDType const op, T t
) {
  auto iter = continuation_actions_t_.find(op);
  auto const& found = iter != continuation_actions_t_.end();

  vt_debug_print(
    verbose, group,
    "GroupManager::triggerContinuationT: op={:x}, found={}, size={}\n",
    op, found, continuation_actions_t_.size()
  );

  if (found) {
    for (auto&& elm : iter->second) {
      elm(t);
    }
  } else {
    auto iter_wait = waiting_cont_.find(op);
    if (iter_wait == waiting_cont_.end()) {
      waiting_cont_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(op),
        std::forward_as_tuple(std::vector<T>{t})
      );
    } else {
      waiting_cont_[op].push_back(t);
    }
  }
}

template <auto f, template <typename Arg> typename Op, typename... Args>
void GroupManager::allreduce(GroupType group, Args&&... args) {
  using namespace collective::reduce::allreduce;

  auto iter = local_collective_group_info_.find(group);
  vtAssert(iter != local_collective_group_info_.end(), "Must exist");

  using DataT = typename function_traits<decltype(f)>::template arg_type<0>;

  using Reducer = Rabenseifner<GroupAllreduceT, DataT, Op, f>;

  // TODO; Save the proxy so it can be deleted afterwards
  auto proxy = theObjGroup()->makeCollective<Reducer>(
    "reducer", collective::reduce::detail::StrongGroup{group},
    std::forward<Args>(args)...
  );

  if (iter->second->is_in_group) {
    auto const this_node = theContext()->getNode();
    auto* ptr = proxy[this_node].get();
    auto id = ptr->id_ - 1;
    ptr->proxy_ = proxy;
    ptr->setFinalHandler(theCB()->makeSend<f>(this_node));
    proxy[this_node].template invoke<&Reducer::allreduce>(id);
  }
}

}} /* end namespace vt::group */

#endif /*INCLUDED_VT_GROUP_GROUP_MANAGER_IMPL_H*/
