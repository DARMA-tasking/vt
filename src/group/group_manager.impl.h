
#if !defined INCLUDED_GROUP_GROUP_MANAGER_IMPL_H
#define INCLUDED_GROUP_GROUP_MANAGER_IMPL_H

#include "config.h"
#include "context/context.h"
#include "group/group_common.h"
#include "group/group_manager.h"
#include "group/id/group_id.h"
#include "group/region/group_region.h"
#include "registry/auto_registry_interface.h"
#include "messaging/message.h"
#include "messaging/active.h"
#include "activefn/activefn.h"
#include "group/group_info.h"

#include <cassert>

namespace vt { namespace group {

template <typename MsgT, ActiveTypedFnType<MsgT> *f>
void GroupManager::sendMsg(GroupType const& group, MsgT* msg) {
  debug_print(
    group, node,
    "GroupManager::sendMsg: group=%llu\n", group
  );

  auto const& this_node = theContext()->getNode();
  auto const& group_node = GroupIDBuilder::getNode(group);
  auto const& group_static = GroupIDBuilder::isStatic(group);
  auto const& group_collective = GroupIDBuilder::isCollective(group);

  assert(
    !group_collective && "Collective groups are not supported"
  );

  HandlerType const& han = auto_registry::makeAutoHandler<MsgT, f>(nullptr);

  msg->setGroup(group);
  msg->user_handler_ = han;

  if (group_node != this_node) {
    theMsg()->sendMsg<MsgT, groupForwardHandler>(group_node, msg);
  } else {
    return sendGroup(msg, true);
  }
}

template <typename MsgT>
void GroupManager::sendGroup(MsgT* msg, bool is_root) {
  auto const& this_node = theContext()->getNode();
  debug_print(
    group, node,
    "GroupManager::sendGroup: group=%llu, is_root=%s\n",
    msg->getGroup(), print_bool(is_root)
  );
  if (is_root) {
    auto iter = local_group_info_.find(msg->getGroup());
    assert(
      iter != local_group_info_.end() && "Local lnfo must exist for group"
    );
    auto& info = *iter->second;
    assert(
      info.is_forward_ && "Must be a forward"
    );
    auto node = info.forward_node_;
    if (node == this_node) {
      return sendGroup(msg, false);
    } else {
      messageRef(msg);
      theMsg()->sendMsg<MsgT, groupSendHandler>(node, msg, [msg]{
        messageDeref(msg);
      });
    }
  } else {
    auto iter = remote_group_info_.find(msg->getGroup());
    messageRef(msg);

    if (iter != remote_group_info_.end()) {
      auto& info = *iter->second;
      assert(
        !info.is_forward_ && "Must be a forward"
      );
      assert(
        info.default_spanning_tree_ != nullptr && "Must have spanning tree"
      );

      info.default_spanning_tree_->foreachChild([msg](NodeType child) {
        auto const& this_node = theContext()->getNode();
        if (child != this_node) {
          debug_print(
            group, node,
            "GroupManager::tree foreach: sending to child=%d\n", child
          );
          messageRef(msg);
          theMsg()->sendMsg<MsgT, groupSendHandler>(child, msg, [msg]{
            messageDeref(msg);
          });
        }
      });
    }

    auto user_fn = auto_registry::getAutoHandler(msg->user_handler_);
    user_fn(reinterpret_cast<BaseMessage*>(msg));
    messageDeref(msg);
  }
}

template <typename MsgT>
/*static*/ void GroupManager::groupForwardHandler(MsgT* msg) {
  theGroup()->sendGroup(msg, true);
}

template <typename MsgT>
/*static*/ void GroupManager::groupSendHandler(MsgT* msg) {
  theGroup()->sendGroup(msg, false);
}

}} /* end namespace vt::group */

#endif /*INCLUDED_GROUP_GROUP_MANAGER_IMPL_H*/
