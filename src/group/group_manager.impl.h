
#if !defined INCLUDED_GROUP_GROUP_MANAGER_IMPL_H
#define INCLUDED_GROUP_GROUP_MANAGER_IMPL_H

#include "config.h"
#include "context/context.h"
#include "group/group_common.h"
#include "group/group_manager.h"
#include "group/id/group_id.h"
#include "group/region/group_region.h"
#include "registry/auto/auto_registry_interface.h"
#include "messaging/message.h"
#include "messaging/active.h"
#include "activefn/activefn.h"
#include "group/group_info.h"

namespace vt { namespace group {

template <typename T>
/*static*/ GroupManager::ActionContainerTType<T>
GroupManager::continuation_actions_t_ = {};

template <typename T>
/*static*/ std::unordered_map<RemoteOperationIDType,std::vector<T>>
GroupManager::waiting_cont_ = {};

template <typename T>
RemoteOperationIDType GroupManager::registerContinuationT(ActionTType<T> act) {
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
    "GroupManager::triggerContinuationT: op={:x}, found={}\n",
    op, found
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
