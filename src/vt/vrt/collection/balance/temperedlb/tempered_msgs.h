/*
//@HEADER
// *****************************************************************************
//
//                               tempered_msgs.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_TEMPEREDLB_TEMPERED_MSGS_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_TEMPEREDLB_TEMPERED_MSGS_H

#include "vt/config.h"

#include INCLUDE_FMT_FORMAT

#include <unordered_map>

namespace vt::vrt::collection::lb {

using BytesType        = double;

struct ClusterInfo {
  LoadType load = 0;
  BytesType bytes = 0;
  double intra_send_vol = 0, intra_recv_vol = 0;
  std::unordered_map<NodeType, double> inter_send_vol, inter_recv_vol;
  NodeType home_node = uninitialized_destination;
  BytesType edge_weight = 0;
  BytesType max_object_working_bytes = 0;
  BytesType max_object_working_bytes_outside = 0;
  BytesType max_object_serialized_bytes = 0;
  BytesType max_object_serialized_bytes_outside = 0;
  BytesType cluster_footprint = 0;

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | load | bytes | intra_send_vol | intra_recv_vol;
    s | inter_send_vol | inter_recv_vol;
    s | home_node | edge_weight;
    s | max_object_working_bytes;
    s | max_object_working_bytes_outside;
    s | max_object_serialized_bytes;
    s | max_object_serialized_bytes_outside;
    s | cluster_footprint;
  }
};

struct NodeInfo {
  LoadType load = 0;
  LoadType work = 0;
  double inter_send_vol = 0, inter_recv_vol = 0;
  double intra_send_vol = 0, intra_recv_vol = 0;
  double shared_vol = 0;

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | load | work;
    s | inter_send_vol | inter_recv_vol;
    s | intra_send_vol | intra_recv_vol;
    s | shared_vol;
  }
};

using ClusterSummaryType = std::unordered_map<SharedIDType, ClusterInfo>;
using RankSummaryType = std::tuple<BytesType, ClusterSummaryType>;

inline auto format_as(ClusterInfo const& e) {
  auto fmt_str = "(load={},bytes={},intra=({},{})),home={},edge={}";
  return fmt::format(
    fmt_str, e.load, e.bytes, e.intra_send_vol, e.intra_recv_vol,
    e.home_node, e.edge_weight
  );
}

} /* end namespace vt::vrt::collection::lb */

namespace vt { namespace vrt { namespace collection { namespace balance {

struct LoadMsg : vt::Message {
  using MessageParentType = vt::Message;
  vt_msg_serialize_required(); // node_load_

  using NodeClusterSummaryType =
    std::unordered_map<NodeType, lb::RankSummaryType>;
  using NodeInfoType = std::unordered_map<NodeType, lb::NodeInfo>;

  LoadMsg() = default;
  LoadMsg(NodeType in_from_node, NodeInfoType const& in_node_info)
    : from_node_(in_from_node), node_info_(in_node_info)
  { }

  NodeInfoType const& getNodeInfo() const {
    return node_info_;
  }

  NodeClusterSummaryType const& getNodeClusterSummary() const {
    return node_cluster_summary_;
  }

  void addNodeInfo(NodeType node, lb::NodeInfo info) {
    node_info_[node] = info;
  }

  void addNodeClusters(
    NodeType node,
    lb::BytesType rank_working_bytes,
    lb::ClusterSummaryType summary
  ) {
    node_cluster_summary_[node] = std::make_tuple(rank_working_bytes, summary);
  }

  NodeType getFromNode() const { return from_node_; }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);
    s | from_node_;
    s | node_info_;
    s | node_cluster_summary_;
  }

private:
  NodeType from_node_     = uninitialized_destination;
  NodeInfoType node_info_ = {};
  NodeClusterSummaryType node_cluster_summary_ = {};
};

struct LoadMsgAsync : LoadMsg {
  using MessageParentType = LoadMsg;
  vt_msg_serialize_if_needed_by_parent();

  LoadMsgAsync() = default;
  LoadMsgAsync(
    NodeType in_from_node, NodeInfoType const& in_node_info, int round
  )
    : LoadMsg(in_from_node, in_node_info), round_(round)
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
  using ObjsType = std::unordered_map<lb::BaseLB::ObjIDType, LoadType>;

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

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_TEMPEREDLB_TEMPERED_MSGS_H*/
