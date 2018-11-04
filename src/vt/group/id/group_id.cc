
#include "vt/config.h"
#include "vt/group/group_common.h"
#include "vt/group/id/group_id.h"
#include "vt/utils/bits/bits_common.h"

namespace vt { namespace group {

/*static*/ GroupType GroupIDBuilder::createGroupID(
  GroupIDType const& id, NodeType const& node, bool const& is_collective,
  bool const& is_static
) {
  auto const& set_node = !is_collective ? node : group_collective_node;
  GroupType new_group = 0;

  setIsCollective(new_group, is_collective);
  setIsStatic(new_group, is_static);
  setNode(new_group, set_node);
  setID(new_group, id);

  return new_group;
}

/*static*/ void GroupIDBuilder::setIsCollective(
  GroupType& group, bool const& is_coll
) {
  BitPackerType::boolSetField<eGroupIDBits::Collective>(group, is_coll);
}

/*static*/ void GroupIDBuilder::setIsStatic(
  GroupType& group, bool const& is_static
) {
  BitPackerType::boolSetField<eGroupIDBits::Static>(group, is_static);
}

/*static*/ void GroupIDBuilder::setNode(
  GroupType& group, NodeType const& node
) {
  BitPackerType::setField<eGroupIDBits::Node, group_node_num_bits>(group, node);
}

/*static*/ void GroupIDBuilder::setID(
  GroupType& group, GroupIDType const& id
) {
  BitPackerType::setField<eGroupIDBits::ID, group_id_num_bits>(group, id);
}

/*static*/ bool GroupIDBuilder::isCollective(GroupType const& group) {
  return BitPackerType::boolGetField<eGroupIDBits::Collective>(group);
}

/*static*/ bool GroupIDBuilder::isStatic(GroupType const& group) {
  return BitPackerType::boolGetField<eGroupIDBits::Static>(group);
}

/*static*/ NodeType GroupIDBuilder::getNode(GroupType const& group) {
  return BitPackerType::getField<
    eGroupIDBits::Node, group_node_num_bits, NodeType
  >(group);
}

/*static*/ GroupIDType GroupIDBuilder::getGroupID(GroupType const& group) {
  return BitPackerType::getField<
    eGroupIDBits::ID, group_id_num_bits, GroupIDType
  >(group);
}



}} /* end namespace vt::group */
