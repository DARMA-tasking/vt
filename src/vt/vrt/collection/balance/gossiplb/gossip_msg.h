/*
//@HEADER
// *****************************************************************************
//
//                                 gossip_msg.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_GOSSIPLB_GOSSIP_MSG_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_GOSSIPLB_GOSSIP_MSG_H

#include "vt/config.h"

#include <vector>
#include <unordered_map>

namespace vt { namespace vrt { namespace collection { namespace balance {

struct GossipMsg : vt::Message {
  using MessageParentType = vt::Message;
  vt_msg_serialize_required(); // node_load_

  using NodeLoadType = std::unordered_map<NodeType, lb::BaseLB::LoadType>;

  GossipMsg() = default;
  GossipMsg(NodeType in_from_node, NodeLoadType const& in_node_load)
    : from_node_(in_from_node), node_load_(in_node_load)
  { }

  NodeLoadType const& getNodeLoad() const {
    return node_load_;
  }

  void addNodeLoad(NodeType node, lb::BaseLB::LoadType load) {
    node_load_[node] = load;
  }

  NodeType getFromNode() const { return from_node_; }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);
    s | from_node_;
    s | node_load_;
  }

private:
  NodeType from_node_     = uninitialized_destination;
  NodeLoadType node_load_ = {};
};

struct GossipMsgAsync : GossipMsg {
  using MessageParentType = GossipMsg;
  vt_msg_serialize_if_needed_by_parent();

  GossipMsgAsync() = default;
  GossipMsgAsync(
    NodeType in_from_node, NodeLoadType const& in_node_load, int round
  )
    : GossipMsg(in_from_node, in_node_load), round_(round)
  { }

  uint8_t getRound() const {
    return round_;
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);
    s | round_;
  }

private:
  int round_;
};

struct LazyMigrationMsg : SerializeRequired<
  vt::Message,
  LazyMigrationMsg
> {
  using MessageParentType = SerializeRequired<
    vt::Message,
    LazyMigrationMsg
  >;
  using ObjsType = std::unordered_map<lb::BaseLB::ObjIDType, lb::BaseLB::LoadType>;

  LazyMigrationMsg() = default;
  LazyMigrationMsg(NodeType in_to_node, ObjsType const& in_objs)
    : to_node_(in_to_node), objs_(in_objs)
  { }

  ObjsType const& getObjSet() const {
    return objs_;
  }

  NodeType getToNode() const { return to_node_; }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);
    s | to_node_;
    s | objs_;
  }

private:
  NodeType to_node_ = uninitialized_destination;
  ObjsType objs_  = {};
};

struct RejectionStats {
  RejectionStats() = default;
  RejectionStats(int n_rejected, int n_transfers)
    : n_rejected_(n_rejected), n_transfers_(n_transfers) { }

  friend RejectionStats operator+(RejectionStats a1, RejectionStats const& a2) {
    a1.n_rejected_ += a2.n_rejected_;
    a1.n_transfers_ += a2.n_transfers_;

    return a1;
  }

  int n_rejected_ = 0;
  int n_transfers_ = 0;
};

struct GossipRejectionStatsMsg : collective::ReduceTMsg<RejectionStats> {
  GossipRejectionStatsMsg() = default;
  GossipRejectionStatsMsg(int n_rejected, int n_transfers)
    : ReduceTMsg<RejectionStats>(RejectionStats(n_rejected, n_transfers))
  { }
  GossipRejectionStatsMsg(RejectionStats&& rs)
    : ReduceTMsg<RejectionStats>(std::move(rs))
  { }
};

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_GOSSIPLB_GOSSIP_MSG_H*/
