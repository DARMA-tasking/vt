
#if ! defined __RUNTIME_TRANSPORT_EVENT_MSGS__
#define __RUNTIME_TRANSPORT_EVENT_MSGS__

#include "configs/types/types_common.h"
#include "message.h"

namespace vt {

struct EventCheckFinishedMsg : ShortMessage {
  EventType event_ = 0, event_back_ = 0;
  NodeType sent_from_node_ = 0;

  EventCheckFinishedMsg(
    EventType const& event_in, NodeType const& in_sent_from_node,
    EventType const& event_back_in
  )
    : ShortMessage(), event_(event_in), event_back_(event_back_in),
      sent_from_node_(in_sent_from_node)
  { }
};

struct EventFinishedMsg : ShortMessage {
  EventType event_ = 0;
  EventType event_back_ = 0;

  EventFinishedMsg(EventType const& event_in, EventType const& event_back_in)
    : ShortMessage(), event_(event_in), event_back_(event_back_in)
  { }
};

extern HandlerType event_finished_han;
extern HandlerType check_event_finished_han;

} //end namespace vt

#endif /*__RUNTIME_TRANSPORT_EVENT_MSGS__*/
