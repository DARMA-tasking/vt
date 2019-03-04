/*
//@HEADER
// ************************************************************************
//
//                          group_manager.impl.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_GROUP_GROUP_MANAGER_IMPL_H
#define INCLUDED_GROUP_GROUP_MANAGER_IMPL_H

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

namespace vt { namespace group {

template <typename T>
/*static*/ GroupManager::ActionContainerTType<T>
GroupManager::continuation_actions_t_ = {};

template <typename T>
/*static*/ std::unordered_map<RemoteOperationIDType,std::vector<T>>
GroupManager::waiting_cont_ = {};

template <typename T>
void GroupManager::pushCleanupAction() {
  // Push the typeless cleanup actions
  if (continuation_actions_t_<T>.size() == 0) {
    cleanup_actions_.push_back([]{ continuation_actions_t_<T>.clear(); });
  }
  if (waiting_cont_<T>.size() == 0) {
    cleanup_actions_.push_back([]{ waiting_cont_<T>.clear(); });
  }
}

template <typename T>
RemoteOperationIDType GroupManager::registerContinuationT(ActionTType<T> act) {
  pushCleanupAction<T>();

  RemoteOperationIDType next_id = cur_id_++;
  continuation_actions_t_<T>.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(next_id),
    std::forward_as_tuple(ActionListTType<T>{act})
  );
  auto iter = waiting_cont_<T>.find(next_id);
  if (iter != waiting_cont_<T>.end()) {
    for (auto&& elm : iter->second) {
      act(elm);
    }
    waiting_cont_<T>.clear();
  }
  return next_id;
}

template <typename T>
void GroupManager::registerContinuationT(
  RemoteOperationIDType const& op, ActionTType<T> action
) {
  debug_print(
    verbose, group, node,
    "GroupManager::registerContinuationT: op={:x}\n", op
  );

  pushCleanupAction<T>();

  continuation_actions_t_<T>[op].push_back(action);
  auto iter = waiting_cont_<T>.find(op);
  if (iter != waiting_cont_<T>.end()) {
    for (auto&& elm : iter->second) {
      action(elm);
    }
    waiting_cont_<T>.clear();
  }
}

template <typename T>
void GroupManager::triggerContinuationT(
  RemoteOperationIDType const& op, T t
) {
  auto iter = continuation_actions_t_<T>.find(op);
  auto const& found = iter != continuation_actions_t_<T>.end();

  debug_print(
    verbose, group, node,
    "GroupManager::triggerContinuationT: op={:x}, found={}, size={}\n",
    op, found, continuation_actions_t_<T>.size()
  );

  if (found) {
    for (auto&& elm : iter->second) {
      elm(t);
    }
  } else {
    auto iter_wait = waiting_cont_<T>.find(op);
    if (iter_wait == waiting_cont_<T>.end()) {
      waiting_cont_<T>.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(op),
        std::forward_as_tuple(std::vector<T>{t})
      );
    } else {
      waiting_cont_<T>[op].push_back(t);
    }
  }
}

}} /* end namespace vt::group */

#endif /*INCLUDED_GROUP_GROUP_MANAGER_IMPL_H*/
