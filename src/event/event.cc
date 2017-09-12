
#include "event.h"
#include "active.h"

namespace vt {

bool ParentEvent::test_ready() {
  bool ready = true;
  for (auto&& e : events) {
    ready &=
      theEvent->test_event_complete(e) ==
      AsyncEvent::EventStateType::EventReady;
  }
  if (ready) {
    events.clear();
  }
  return ready;
}

void AsyncEvent::EventHolder::make_ready_trigger() {
  //printf("make_ready_trigger\n");
  event->set_ready();
  execute_actions();
  theEvent->container_.erase(event->event_id);
}

EventType AsyncEvent::attach_action(EventType const& event, ActionType callable) {
  auto const& this_node = theContext->get_node();
  auto const& event_id = create_normal_event_id(this_node);
  auto& holder = get_event_holder(event_id);
  NormalEvent& norm_event = *static_cast<NormalEvent*>(holder.get_event());

  auto trigger = [=]{
    callable();
  };

  auto const& event_state = test_event_complete(event);
  auto const& this_event_owning_node = get_owning_node(event_id);

  debug_print(
    event, node,
    "theEvent: event=%lld, newevent=%lld, state=%d, "
    "newevent_owning_node=%d, this_node=%d\n",
    event, event_id, event_state, this_event_owning_node, this_node
  );

  switch (event_state) {
  case EventStateType::EventReady:
    trigger();
    break;
  case EventStateType::EventWaiting:
    this->get_event_holder(event).attach_action(
      trigger
    );
    holder.make_ready_trigger();
    break;
  case EventStateType::EventRemote: {
    // attach event to new id
    holder.attach_action(trigger);

    auto const& owning_node = get_owning_node(event);
    auto msg = new EventCheckFinishedMsg(event, this_node, event_id);

    debug_print(
      event, node,
      "theEvent: event=%lld, newevent=%lld, state=%d sending msg, node=%d\n",
      event, event_id, event_state, this_node
    );

    theMsg->send_msg<EventCheckFinishedMsg, check_event_finished>(
      owning_node, msg, [=]{ delete msg; }
    );
  }
    break;
  default:
    assert(0 && "This should be unreachable");
    break;
  }
  return event_id;
}

/*static*/ void AsyncEvent::event_finished(EventFinishedMsg* msg) {
  auto const& complete = theEvent->test_event_complete(msg->event_back_);

  assert(
    complete == AsyncEvent::EventStateType::EventWaiting and
    "Event must be waiting since it depends on this finished event"
  );

  auto& holder = theEvent->get_event_holder(msg->event_back_);
  holder.make_ready_trigger();
}

/*static*/ void AsyncEvent::check_event_finished(EventCheckFinishedMsg* msg) {
  auto const& event = msg->event_;
  auto const& node = theEvent->get_owning_node(event);

  assert(
    node == theContext->get_node() and "Node must be identical"
  );

  auto send_back_fun = [=]{
    auto msg_send = new EventFinishedMsg(event, msg->event_back_);
    auto send_back = theEvent->get_owning_node(msg->event_back_);
    assert(send_back == msg->sent_from_node_);
    theMsg->send_msg<EventFinishedMsg, event_finished>(
      send_back, msg_send, [=]{ delete msg_send; }
    );
  };

  auto const& is_complete = theEvent->test_event_complete(event);

  debug_print(
    event, node,
    "check_event_finished_han:: event=%lld, node=%lld, "
    "this_node=%d, complete=%d, sent_from_node=%d\n",
    event, node, this_node, is_complete, msg->sent_from_node
  );

  if (is_complete == AsyncEvent::EventStateType::EventReady) {
    send_back_fun();
  } else {
    assert(
      is_complete == AsyncEvent::EventStateType::EventWaiting and
      "Must be waiting if not ready"
    );
    /*ignore return event*/ theEvent->attach_action(event, send_back_fun);
  }
}

bool AsyncEvent::scheduler() {
  theEvent->test_events_trigger(mpi_event_tag);
  theEvent->test_events_trigger(normal_event_tag);
  return false;
}

bool AsyncEvent::is_local_term() {
  return event_container_[mpi_event_tag].size() == 0;
}

} //end namespace vt
