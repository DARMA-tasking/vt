/*
//@HEADER
// *****************************************************************************
//
//                                    msg.h
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

#if !defined INCLUDED_VT_TOPOS_LOCATION_MESSAGE_MSG_H
#define INCLUDED_VT_TOPOS_LOCATION_MESSAGE_MSG_H

#include "vt/config.h"
#include "vt/topos/location/location_common.h"
#include "vt/messaging/message.h"

namespace vt { namespace location {

template <typename EntityID>
struct LocationMsg : vt::Message {
  using MessageParentType = vt::Message;
  vt_msg_serialize_prohibited();

  LocInstType loc_man_inst = 0;
  EntityID entity{};
  LocEventID loc_event = no_location_event_id;
  NodeType ask_node = uninitialized_destination;
  NodeType home_node = uninitialized_destination;
  NodeType resolved_node = uninitialized_destination;

  LocationMsg(
    LocInstType const& in_loc_man_inst, EntityID const& in_entity,
    LocEventID const& in_loc_event, NodeType const& in_ask_node,
    NodeType in_home_node
  ) : loc_man_inst(in_loc_man_inst), entity(in_entity), loc_event(in_loc_event),
      ask_node(in_ask_node), home_node(in_home_node)
  { }

  LocationMsg(
    LocInstType const& in_loc_man_inst, EntityID const& in_entity,
    NodeType const& in_ask_node, NodeType const& in_home_node,
    NodeType in_resolved
  ) : loc_man_inst(in_loc_man_inst), entity(in_entity), ask_node(in_ask_node),
      home_node(in_home_node), resolved_node(in_resolved)
  { }

  void setResolvedNode(NodeType const& node) {
    resolved_node = node;
  }
};

template <typename EntityID, typename ActiveMessageT>
struct EntityMsg : ActiveMessageT {
  using MessageParentType = ActiveMessageT;
  vt_msg_serialize_if_needed_by_parent();

  EntityMsg() = default;
  EntityMsg(EntityID const& in_entity_id, NodeType const& in_home_node)
    : ActiveMessageT(), entity_id_(in_entity_id), home_node_(in_home_node)
  { }

  void setEntity(EntityID const& entity) { entity_id_ = entity; }
  EntityID getEntity() const { return entity_id_; }
  void setHomeNode(NodeType const& node) { home_node_ = node; }
  NodeType getHomeNode() const { return home_node_; }
  void setLocFromNode(NodeType const& node) { loc_from_node_ = node; }
  NodeType getLocFromNode() const { return loc_from_node_; }
  void setLocInst(LocInstType const& inst) { loc_man_inst_ = inst; }
  LocInstType getLocInst() const { return loc_man_inst_;  }
  bool hasHandler() const { return handler_ != uninitialized_handler; }
  void setHandler(HandlerType const han) { handler_ = han; }
  HandlerType getHandler() const { return handler_; }
  void setSerialize(bool const is_serialize) { serialize_ = is_serialize; }
  bool getSerialize() const { return serialize_; }
  void incHops() { hops_ += 1; }
  int16_t getHops() const { return hops_; }
  void setAskNode(NodeType const& node) { ask_node_ = node; }
  NodeType getAskNode() const { return ask_node_; }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);
    s | entity_id_;
    s | home_node_;
    s | loc_from_node_;
    s | loc_man_inst_;
    s | handler_;
    s | serialize_;
    s | hops_;
    s | ask_node_;
  }

private:
  EntityID entity_id_{};
  NodeType home_node_ = uninitialized_destination;
  NodeType loc_from_node_ = uninitialized_destination;
  LocInstType loc_man_inst_ = no_loc_inst;
  HandlerType handler_ = uninitialized_handler;
  bool serialize_ = false;
  int16_t hops_ = 0;
  NodeType ask_node_ =  uninitialized_destination;
};

}}  // end namespace vt::location

namespace vt {

template <typename EntityID, typename ActiveMessageT>
using LocationRoutedMsg = location::EntityMsg<EntityID, ActiveMessageT>;

}  // end namespace::vt

#endif /*INCLUDED_VT_TOPOS_LOCATION_MESSAGE_MSG_H*/
