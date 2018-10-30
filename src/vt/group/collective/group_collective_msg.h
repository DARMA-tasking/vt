
#if !defined INCLUDED_GROUP_GROUP_COLLECTIVE_MSG_H
#define INCLUDED_GROUP_GROUP_COLLECTIVE_MSG_H

#include "config.h"
#include "group/group_common.h"
#include "group/msg/group_msg.h"
#include "messaging/message.h"

#include <cstdlib>

namespace vt { namespace group {

template <typename MsgT>
struct GroupCollectiveInfoMsg : MsgT {
  using CountType = int32_t;

  GroupCollectiveInfoMsg() = default;
  GroupCollectiveInfoMsg(
    GroupType const& in_group, RemoteOperationIDType in_op, bool in_is_in_group,
    NodeType const& in_subtree,
    NodeType const& in_child_node = uninitialized_destination,
    CountType const& level = 0, CountType const& extra_nodes = 0
  ) : MsgT(in_group, in_op), is_in_group(in_is_in_group),
      child_node_(in_child_node), subtree_size_(in_subtree),
      extra_nodes_(extra_nodes), level_(level)
  { }

  NodeType getChild() const { return child_node_; }
  NodeType getSubtreeSize() const { return subtree_size_; }
  bool isStatic() const { return is_static_; }
  bool isInGroup() const { return is_in_group; }
  CountType getExtraNodes() const { return extra_nodes_; }
  CountType getLevel() const { return level_; }

private:
  bool is_in_group = false;
  bool is_static_ = true;
  NodeType child_node_ = uninitialized_destination;
  NodeType subtree_size_ = 0;
  CountType extra_nodes_ = 0;
  CountType level_ = 0;
};

using GroupCollectiveMsg = GroupCollectiveInfoMsg<GroupMsg<::vt::Message>>;

}} /* end namespace vt::group */

#endif /*INCLUDED_GROUP_GROUP_COLLECTIVE_MSG_H*/
