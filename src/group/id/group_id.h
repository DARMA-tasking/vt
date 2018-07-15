
#if !defined INCLUDED_GROUP_ID_GROUP_ID_H
#define INCLUDED_GROUP_ID_GROUP_ID_H

#include "config.h"
#include "group/group_common.h"
#include "utils/bits/bits_common.h"

namespace vt { namespace group {

static constexpr BitCountType const group_is_collective_num_bits = 1;
static constexpr BitCountType const group_is_static_num_bits = 1;
static constexpr BitCountType const group_node_num_bits =
    BitCounterType<NodeType>::value;
static constexpr BitCountType const group_id_num_bits =
    BitCounterType<GroupIDType>::value;
static constexpr NodeType const group_collective_node = -1;

enum eGroupIDBits {
  Collective = 0,
  Static     = eGroupIDBits::Collective + group_is_collective_num_bits,
  Node       = eGroupIDBits::Static     + group_is_static_num_bits,
  ID         = eGroupIDBits::Node       + group_node_num_bits
};

struct GroupIDBuilder {
  static GroupType createGroupID(
    GroupIDType const& id, NodeType const& node,
    bool const& is_collective = false, bool const& is_static = true
  );

  static void setIsCollective(GroupType& group, bool const& is_coll);
  static void setIsStatic(GroupType& group, bool const& is_static);
  static void setNode(GroupType& group, NodeType const& node);
  static void setID(GroupType& group, GroupIDType const& id);
  static bool isCollective(GroupType const& group);
  static bool isStatic(GroupType const& group);
  static NodeType getNode(GroupType const& group);
  static GroupIDType getGroupID(GroupType const& group);
};

}} /* end namespace vt::group */

#endif /*INCLUDED_GROUP_ID_GROUP_ID_H*/
