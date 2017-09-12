
#if ! defined __RUNTIME_TRANSPORT_EVENT_MSGS__
#define __RUNTIME_TRANSPORT_EVENT_MSGS__

#include "common.h"
#include "message.h"

namespace runtime {

struct EventCheckFinishedMsg : ShortMessage {
  EventType event = 0, event_back = 0;
  NodeType sent_from_node = 0;

  EventCheckFinishedMsg(
    EventType const& event_in, NodeType const& in_sent_from_node,
    EventType const& event_back_in
  )
    : ShortMessage(), event(event_in), sent_from_node(in_sent_from_node),
      event_back(event_back_in)
  { }
};

struct EventFinishedMsg : ShortMessage {
  EventType event = 0;
  EventType event_back = 0;

  EventFinishedMsg(EventType const& event_in, EventType const& event_back_in)
    : ShortMessage(), event(event_in), event_back(event_back_in)
  { }
};

extern HandlerType event_finished_han;
extern HandlerType check_event_finished_han;

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_EVENT_MSGS__*/
