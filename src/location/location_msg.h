
#if ! defined __RUNTIME_TRANSPORT_LOCATION_MSG__
#define __RUNTIME_TRANSPORT_LOCATION_MSG__

#include "common.h"
#include "message.h"
#include "location_common.h"

namespace vt { namespace location {

template <typename EntityID>
struct LocationMsg : vt::Message {
  LocManInstType loc_man_inst = LocManInstType::InvalidLocManInst;
  EntityID entity{};
  LocEventID loc_event = no_location_event_id;
  NodeType ask_node = uninitialized_destination;
  NodeType home_node = uninitialized_destination;
  NodeType resolved_node = uninitialized_destination;

  LocationMsg(
    LocManInstType const& in_loc_man_inst, EntityID const& in_entity,
    LocEventID const& in_loc_event, NodeType const& in_ask_node,
    NodeType in_home_node
  ) : loc_man_inst(in_loc_man_inst), entity(in_entity), loc_event(in_loc_event),
      ask_node(in_ask_node), home_node(in_home_node)
  { }

  void setResolvedNode(NodeType const& node) {
    resolved_node = node;
  }
};

}} // end namespace vt::location

#endif /*__RUNTIME_TRANSPORT_LOCATION_MSG__*/
