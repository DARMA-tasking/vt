
#include "event_id.h"

namespace vt { namespace event {

/*static*/ EventType EventIDManager::makeEvent(
  EventIdentifierType const& id, NodeType const& node
) {
  EventType new_event_id = 0;
  EventIDManager::setEventNode(new_event_id, node);
  EventIDManager::setEventIdentifier(new_event_id, id);

  debug_print(
    event, node,
    "EventIDManager::makeEvent: id={}, node={}\n", id, node
  );

  return new_event_id;
}

/*static*/ NodeType EventIDManager::getEventNode(EventType const& event) {
  return BitPackerType::getField<
    EventIDBitsType::Node, node_num_bits, NodeType
  >(event);
}

/*static*/ void EventIDManager::setEventNode(
  EventType& event, NodeType const& node
) {
  BitPackerType::setField<EventIDBitsType::Node, node_num_bits>(event, node);
}

/*static*/ void EventIDManager::setEventIdentifier(
  EventType& event, EventIdentifierType const& id
) {
  BitPackerType::setField<
    EventIDBitsType::EventIdent, event_identifier_num_bits
  >(event, id);
}

/*static*/ EventIdentifierType EventIDManager::getEventIdentifier(
  EventType const& event
) {
  return BitPackerType::getField<
    EventIDBitsType::EventIdent, event_identifier_num_bits, EventIdentifierType
  >(event);
}

}} //end namespace vt::event
