/*
//@HEADER
// *****************************************************************************
//
//                                  lb_comm.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_COMM_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_COMM_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/lb_common.h"

#include <unordered_map>

namespace vt { namespace vrt { namespace collection { namespace balance {

enum struct CommCategory : int8_t {
  SendRecv = 1,
  CollectionToNode = 2,
  NodeToCollection = 3,
  Broadcast = 4,
  CollectionToNodeBcast = 5,
  NodeToCollectionBcast = 6,
  CollectiveToCollectionBcast = 7,
  LocalInvoke = 8
};

inline NodeType objGetNode(ElementIDStruct const id) {
  return id.curr_node;
}

struct LBCommKey {

  struct CollectionTag { };
  struct CollectionToNodeTag { };
  struct NodeToCollectionTag { };

  LBCommKey() = default;
  LBCommKey(LBCommKey const&) = default;
  LBCommKey(LBCommKey&&) = default;
  LBCommKey& operator=(LBCommKey const&) = default;

  LBCommKey(
    CollectionTag,
    ElementIDStruct from, ElementIDStruct to,
    bool bcast
  ) : from_(from), to_(to),
      cat_(bcast ? CommCategory::Broadcast : CommCategory::SendRecv)
  { }
  LBCommKey(
    CollectionToNodeTag,
    ElementIDStruct from, NodeType to,
    bool bcast
  ) : from_(from), nto_(to),
      cat_(bcast ? CommCategory::CollectionToNodeBcast : CommCategory::CollectionToNode)
  { }
  LBCommKey(
    NodeToCollectionTag,
    NodeType from, ElementIDStruct to,
    bool bcast
  ) : to_(to), nfrom_(from),
      cat_(bcast ? CommCategory::NodeToCollectionBcast : CommCategory::NodeToCollection)
  { }

  ElementIDStruct from_ = { no_element_id, uninitialized_destination, uninitialized_destination };
  ElementIDStruct to_   = { no_element_id, uninitialized_destination, uninitialized_destination };

  ElementIDStruct edge_id_ = { no_element_id, uninitialized_destination, uninitialized_destination };
  NodeType nfrom_          = uninitialized_destination;
  NodeType nto_            = uninitialized_destination;
  CommCategory  cat_       = CommCategory::SendRecv;

  ElementIDStruct fromObj()    const { return from_; }
  ElementIDStruct toObj()      const { return to_; }
  ElementIDType fromNode()     const { return nfrom_; }
  ElementIDType toNode()       const { return nto_; }
  ElementIDStruct edgeID()     const { return edge_id_; }

  bool selfEdge() const { return cat_ == CommCategory::SendRecv and from_ == to_; }
  bool offNode() const {
    if (cat_ == CommCategory::SendRecv) {
      return objGetNode(from_) != objGetNode(to_);
    } else if (cat_ == CommCategory::CollectionToNode) {
      return objGetNode(from_) != nto_;
    } else if (cat_ == CommCategory::NodeToCollection) {
      return objGetNode(to_) != nfrom_;
    } else {
      return true;
    }
  }
  bool onNode() const { return !offNode(); }

  bool operator==(LBCommKey const& k) const {
    return
      k.from_  ==  from_ and k.to_  ==  to_ and
      k.nfrom_ == nfrom_ and k.nto_ == nto_ and
      k.cat_   == cat_;
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | from_ | to_ | nfrom_ | nto_ | cat_ | edge_id_;
  }
};

// Set the types for the communication graph
using CommKeyType   = LBCommKey;
using CommBytesType = double;

struct CommVolume {
  CommBytesType bytes = 0.0;
  uint64_t messages = 0;

  void receiveMsg(double b) {
    messages++;
    bytes += b;
  }

  void operator+=(const CommVolume &rhs) {
    bytes += rhs.bytes;
    messages += rhs.messages;
  }

  template <typename SerializerT>
  void serialize(SerializerT &s) {
    s | bytes | messages;
  }
};

using CommMapType   = std::unordered_map<CommKeyType,CommVolume>;

}}}} /* end namespace vt::vrt::collection::balance */

namespace std {

using CommCategoryType    = vt::vrt::collection::balance::CommCategory;
using LBCommKeyType       = vt::vrt::collection::balance::LBCommKey;
using ElementIDStructType = vt::vrt::collection::balance::ElementIDStruct;

template <>
struct hash<CommCategoryType> {
  size_t operator()(CommCategoryType const& in) const {
    using LBUnderType = typename std::underlying_type<CommCategoryType>::type;
    auto const val = static_cast<LBUnderType>(in);
    return std::hash<LBUnderType>()(val);
  }
};

template <>
struct hash<LBCommKeyType> {
  size_t operator()(LBCommKeyType const& in) const {
    return std::hash<uint64_t>()(
      std::hash<ElementIDStructType>()(in.from_) ^
      std::hash<ElementIDStructType>()(in.to_) ^ in.nfrom_ ^ in.nto_
    );
  }
};

} /* end namespace std */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_COMM_H*/
