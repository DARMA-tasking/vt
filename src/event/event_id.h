
#if ! defined __RUNTIME_TRANSPORT_EVENT_ID__
#define __RUNTIME_TRANSPORT_EVENT_ID__

#include "config.h"
#include "messaging/message.h"

namespace vt { namespace event {

using EventIdentifierType = int32_t;

static constexpr BitCountType const event_identifier_num_bits = 32;

enum eEventIDBits {
  Node       = 0,
  EventIdent = eEventIDBits::Node       + node_num_bits
};

struct EventIDManager {
  using EventIDBitsType = eEventIDBits;

  EventIDManager() = default;

  static EventType makeEvent(EventIdentifierType const& id, NodeType const& node);
  static NodeType getEventNode(EventType const& event);
  static void setEventNode(EventType& event, NodeType const& node);
  static void setEventIdentifier(EventType& event, EventIdentifierType const& id);
  static EventIdentifierType getEventIdentifier(EventType const& event);

};

}} //end namespace vt::event

#endif /*__RUNTIME_TRANSPORT_EVENT_ID__*/
