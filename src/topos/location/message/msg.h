
#if !defined INCLUDED_TOPOS_LOCATION_MESSAGE_MSG_H
#define INCLUDED_TOPOS_LOCATION_MESSAGE_MSG_H

#include "config.h"
#include "topos/location/location_common.h"
#include "messaging/message.h"

namespace vt { namespace location {

template <typename EntityID>
struct LocationMsg : vt::Message {
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

  void setResolvedNode(NodeType const& node) {
    resolved_node = node;
  }
};

template <typename EntityID, typename ActiveMessageT>
struct EntityMsg : ActiveMessageT {
  // By default, the `EntityMsg' is byte copyable for serialization
  using isByteCopyable = std::true_type;

  EntityMsg() = default;
  EntityMsg(EntityID const& in_entity_id, NodeType const& in_home_node)
    : ActiveMessageT(), entity_id_(in_entity_id), home_node_(in_home_node)
  { }

  void setEntity(EntityID const& entity) { entity_id_ = entity; }
  EntityID getEntity() const { return entity_id_; }
  void setHomeNode(NodeType const& node) { home_node_ = node; }
  NodeType getHomeNode() const { return home_node_; }
  void setLocInst(LocInstType const& inst) { loc_man_inst_ = inst; }
  LocInstType getLocInst() const { return loc_man_inst_;  }
  bool hasHandler() const { return handler_ != uninitialized_handler; }
  void setHandler(HandlerType const& han) { handler_ = han; }
  HandlerType getHandler() const { return handler_; }
  void setSerialize(bool const serialize) { serialize_ = serialize; }
  bool getSerialize() const { return serialize_; }

  // Explicitly write parent serialize so derived classes can have non-byte
  // serializers
  template <typename SerializerT>
  void serializeParent(SerializerT& s) {
    ActiveMessageT::serializeParent(s);
    ActiveMessageT::serializeThis(s);
  }

  template <typename SerializerT>
  void serializeThis(SerializerT& s) {
    s | entity_id_;
    s | home_node_;
    s | loc_man_inst_;
    s | handler_;
    s | serialize_;
  }

private:
  EntityID entity_id_{};
  NodeType home_node_ = uninitialized_destination;
  LocInstType loc_man_inst_ = no_loc_inst;
  HandlerType handler_ = uninitialized_handler;
  bool serialize_ = false;
};

}}  // end namespace vt::location

namespace vt {

template <typename EntityID, typename ActiveMessageT>
using LocationRoutedMsg = location::EntityMsg<EntityID, ActiveMessageT>;

}  // end namespace::vt

#endif /*INCLUDED_TOPOS_LOCATION_MESSAGE_MSG_H*/
