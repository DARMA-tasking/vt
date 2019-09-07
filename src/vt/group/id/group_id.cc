/*
//@HEADER
// *****************************************************************************
//
//                                 group_id.cc
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
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
