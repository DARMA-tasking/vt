
#include "event.h"
#include "active.h"

namespace vt {

bool ParentEvent::testReady() {
  bool ready = true;
  for (auto&& e : events) {
    ready &=
      theEvent->testEventComplete(e) ==
      AsyncEvent::EventStateType::EventReady;
  }
  if (ready) {
    events.clear();
  }
  return ready;
}

void AsyncEvent::EventHolder::makeReadyTrigger() {
  //printf("make_ready_trigger\n");
  event->setReady();
  executeActions();
  theEvent->container_.erase(event->event_id);
}

EventType AsyncEvent::attachAction(EventType const& event, ActionType callable) {
  auto const& this_node = theContext->getNode();
  auto const& event_id = createNormalEventId(this_node);
  auto& holder = getEventHolder(event_id);
  NormalEvent& norm_event = *static_cast<NormalEvent*>(holder.get_event());

  auto trigger = [=]{
    callable();
  };

  auto const& event_state = testEventComplete(event);
  auto const& this_event_owning_node = getOwningNode(event_id);

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
    this->getEventHolder(event).attachAction(
      trigger
    );
    holder.makeReadyTrigger();
    break;
  case EventStateType::EventRemote: {
    // attach event to new id
    holder.attachAction(trigger);

    auto const& owning_node = getOwningNode(event);
    auto msg = new EventCheckFinishedMsg(event, this_node, event_id);

    debug_print(
      event, node,
      "theEvent: event=%lld, newevent=%lld, state=%d sending msg, node=%d\n",
      event, event_id, event_state, this_node
    );

    theMsg->sendMsg<EventCheckFinishedMsg, checkEventFinished>(
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

/*static*/ void AsyncEvent::eventFinished(EventFinishedMsg* msg) {
  auto const& complete = theEvent->testEventComplete(msg->event_back_);

  assert(
    complete == AsyncEvent::EventStateType::EventWaiting and
    "Event must be waiting since it depends on this finished event"
  );

  auto& holder = theEvent->getEventHolder(msg->event_back_);
  holder.makeReadyTrigger();
}

/*static*/ void AsyncEvent::checkEventFinished(EventCheckFinishedMsg* msg) {
  auto const& event = msg->event_;
  auto const& node = theEvent->getOwningNode(event);

  assert(
    node == theContext->getNode() and "Node must be identical"
  );

  auto send_back_fun = [=]{
    auto msg_send = new EventFinishedMsg(event, msg->event_back_);
    auto send_back = theEvent->getOwningNode(msg->event_back_);
    assert(send_back == msg->sent_from_node_);
    theMsg->sendMsg<EventFinishedMsg, eventFinished>(
      send_back, msg_send, [=]{ delete msg_send; }
    );
  };

  auto const& is_complete = theEvent->testEventComplete(event);

  debug_print(
    event, node,
    "checkEventFinishedHan:: event=%lld, node=%lld, "
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
    /*ignore return event*/ theEvent->attachAction(event, send_back_fun);
  }
}

bool AsyncEvent::scheduler() {
  theEvent->testEventsTrigger(mpi_event_tag);
  theEvent->testEventsTrigger(normal_event_tag);
  return false;
}

bool AsyncEvent::isLocalTerm() {
  return event_container_[mpi_event_tag].size() == 0;
}

} //end namespace vt
