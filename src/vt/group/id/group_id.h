/*
//@HEADER
// *****************************************************************************
//
//                                  group_id.h
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

#if !defined INCLUDED_GROUP_ID_GROUP_ID_H
#define INCLUDED_GROUP_ID_GROUP_ID_H

#include "vt/config.h"
#include "vt/group/group_common.h"
#include "vt/utils/bits/bits_common.h"

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
