
#include "transport.h"
#include "event.h"

namespace runtime {

bool
ParentEvent::test_ready() {
  bool ready = true;
  for (auto&& e : events) {
    //debug_print_event("ParentEvent: test_ready() e=%lld\n", e);
    ready &=
      the_event->test_event_complete(e) ==
      AsyncEvent::event_state_t::EventReady;
  }
  if (ready) {
    events.clear();
  }
  return ready;
}

void
AsyncEvent::EventHolder::make_ready_trigger() {
  //printf("make_ready_trigger\n");
  event->set_ready();
  execute_actions();
  the_event->container.erase(event->event_id);
}

event_t
AsyncEvent::attach_action(event_t const& event, action_t callable) {
  auto const& this_node = the_context->get_node();
  auto const& event_id = create_normal_event_id(this_node);
  auto& holder = get_event_holder(event_id);
  NormalEvent& norm_event = *static_cast<NormalEvent*>(holder.get_event());

  auto trigger = [=]{
    callable();
  };

  auto const& event_state = test_event_complete(event);
  auto const& this_event_owning_node = get_owning_node(event_id);

  debug_print_event(
    "the_event: event=%lld, newevent=%lld, state=%d, "
    "newevent_owning_node=%d, this_node=%d\n",
    event, event_id, event_state, this_event_owning_node, this_node
  );

  switch (event_state) {
  case event_state_t::EventReady:
    trigger();
    break;
  case event_state_t::EventWaiting:
    this->get_event_holder(event).attach_action(
      trigger
    );
    holder.make_ready_trigger();
    break;
  case event_state_t::EventRemote: {
    // attach event to new id
    holder.attach_action(trigger);

    auto const& owning_node = get_owning_node(event);
    auto msg = new EventCheckFinishedMsg(event, this_node, event_id);

    debug_print_event(
      "the_event: event=%lld, newevent=%lld, state=%d sending msg, node=%d\n",
      event, event_id, event_state, this_node
    );

    the_msg->send_msg<EventCheckFinishedMsg, check_event_finished>(
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

/*static*/ void
AsyncEvent::event_finished(EventFinishedMsg* msg) {
  auto const& complete = the_event->test_event_complete(msg->event_back);

  assert(
    complete == AsyncEvent::event_state_t::EventWaiting and
    "Event must be waiting since it depends on this finished event"
  );

  auto& holder = the_event->get_event_holder(msg->event_back);
  holder.make_ready_trigger();
}

/*static*/ void
AsyncEvent::check_event_finished(EventCheckFinishedMsg* msg) {
  auto const& event = msg->event;
  auto const& node = the_event->get_owning_node(event);

  assert(
    node == the_context->get_node() and "Node must be identical"
  );

  auto const& this_node = the_context->get_node();

  auto send_back_fun = [=]{
    auto msg_send = new EventFinishedMsg(event, msg->event_back);
    auto send_back = the_event->get_owning_node(msg->event_back);
    assert(send_back == msg->sent_from_node);
    the_msg->send_msg<EventFinishedMsg, event_finished>(
      send_back, msg_send, [=]{ delete msg_send; }
    );
  };

  auto const& is_complete = the_event->test_event_complete(event);

  debug_print_event(
    "check_event_finished_han:: event=%lld, node=%lld, "
    "this_node=%d, complete=%d, sent_from_node=%d\n",
    event, node, this_node, is_complete, msg->sent_from_node
  );

  if (is_complete == AsyncEvent::event_state_t::EventReady) {
    send_back_fun();
  } else {
    assert(
      is_complete == AsyncEvent::event_state_t::EventWaiting and
      "Must be waiting if not ready"
    );
    /*ignore return event*/ the_event->attach_action(event, send_back_fun);
  }
}

} //end namespace runtime
