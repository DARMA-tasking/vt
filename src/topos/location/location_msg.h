
#if !defined INCLUDED_TOPOS_LOCATION_MSG
#define INCLUDED_TOPOS_LOCATION_MSG

#include "config.h"
#include "messaging/message.h"
#include "location_common.h"

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
      ask_node(in_ask_node), home_node(in_home_node) {}

  void setResolvedNode(NodeType const& node) {
    resolved_node = node;
  }
};

template <typename EntityID, typename ActiveMessageT>
struct EntityMsg : ActiveMessageT {
  EntityID entity_id{};
  NodeType home_node = uninitialized_destination;
  LocInstType loc_man_inst = no_loc_inst;
  HandlerType handler = uninitialized_handler;

  EntityMsg() = default;

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | entity_id | home_node | loc_man_inst | handler;
  }

  explicit EntityMsg(EntityID const& in_entity_id, NodeType const& in_home_node)
      : ActiveMessageT(), entity_id(in_entity_id), home_node(in_home_node) {}

  EntityID getEntity() const {
    return entity_id;
  }

  void setLocInst(LocInstType const& inst) {
    loc_man_inst = inst;
  }

  LocInstType getLocInst() const {
    return loc_man_inst;
  }

  void setHandler(HandlerType const& han) {
    handler = han;
  }

  bool hasHandler() const {
    return handler != uninitialized_destination;
  }

  HandlerType getHandler() const {
    return handler;
  }
};

}}  // end namespace vt::location

namespace vt {

template <typename EntityID, typename ActiveMessageT>
using LocationRoutedMsg = location::EntityMsg<EntityID, ActiveMessageT>;

}  // end namespace::vt

#endif  /*INCLUDED_TOPOS_LOCATION_MSG*/
