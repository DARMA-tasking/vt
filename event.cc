
#include "transport.h"
#include "event.h"

namespace runtime {

bool
ParentEvent::test_ready() {
  bool ready = true;
  for (auto&& e : events) {
    //DEBUG_PRINT("ParentEvent: test_ready() e=%lld\n", e);
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

  DEBUG_PRINT(
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

    DEBUG_PRINT(
      "the_event: event=%lld, newevent=%lld, state=%d sending msg, node=%d\n",
      event, event_id, event_state, this_node
    );

    the_msg->send_msg(
      owning_node, check_event_finished_han, msg, [=]{
        delete msg;
      }
    );
  }
    break;
  default:
    assert(0 && "This should be unreachable");
    break;
  }
  return event_id;
}


} //end namespace runtime
