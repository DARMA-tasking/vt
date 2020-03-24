/*
//@HEADER
// *****************************************************************************
//
//                                   event.cc
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#include "vt/event/event.h"
#include "vt/messaging/active.h"

namespace vt { namespace event {

// bool ParentEvent::testReady() {
//   bool ready = true;
//   for (auto&& e : events) {
//     ready &=
//       theEvent()->testEventComplete(e) ==
//       AsyncEvent::EventStateType::EventReady;
//   }
//   if (ready) {
//     events.clear();
//   }
//   return ready;
// }


EventType AsyncEvent::attachAction(EventType const& event, ActionType callable) {
  auto const& this_node = theContext()->getNode();
  auto const& event_id = createNormalEvent(this_node);
  auto& holder = getEventHolder(event_id);

  auto trigger = [=]{
    callable();
  };

  auto const& event_state = testEventComplete(event);
  auto const& this_event_owning_node = getOwningNode(event_id);

  debug_print(
    event, node,
    "theEvent: event={}, newevent={}, state={}, "
    "newevent_owning_node={}, this_node={}\n",
    event, event_id, static_cast<int>(event_state), this_event_owning_node,
    this_node
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
    auto msg = makeSharedMessage<EventCheckFinishedMsg>(
      event, this_node, event_id
    );

    debug_print(
      event, node,
      "theEvent: event={}, newevent={}, state={} sending msg, node={}\n",
      event, event_id, static_cast<int>(event_state), this_node
    );

    theMsg()->sendMsg<EventCheckFinishedMsg, checkEventFinished>(
      owning_node, msg
    );
  }
    break;
  default:
    vtAssert(0, "This should be unreachable");
    break;
  }
  return event_id;
}

/*static*/ void AsyncEvent::eventFinished(EventFinishedMsg* msg) {
  auto const& complete = theEvent()->testEventComplete(msg->event_back_);

  vtAssert(
    complete == AsyncEvent::EventStateType::EventWaiting,
    "Event must be waiting since it depends on this finished event"
  );

  auto& holder = theEvent()->getEventHolder(msg->event_back_);
  holder.makeReadyTrigger();
}

/*static*/ void AsyncEvent::checkEventFinished(EventCheckFinishedMsg* msg) {
  auto const& event = msg->event_;
  auto const& node = theEvent()->getOwningNode(event);

  vtAssert(
    node == theContext()->getNode(), "Node must be identical"
  );

  auto send_back_fun = [=]{
    auto msg_send = makeSharedMessage<EventFinishedMsg>(event, msg->event_back_);
    auto send_back = theEvent()->getOwningNode(msg->event_back_);
    vtAssertExpr(send_back == msg->sent_from_node_);
    theMsg()->sendMsg<EventFinishedMsg, eventFinished>(
      send_back, msg_send
    );
  };

  auto const& is_complete = theEvent()->testEventComplete(event);

  debug_print(
    event, node,
    "checkEventFinishedHan:: event={}, node={}, "
    "this_node={}, complete={}, sent_from_node={}\n",
    event, node, theContext()->getNode(), static_cast<int>(is_complete),
    msg->sent_from_node_
  );

  if (is_complete == AsyncEvent::EventStateType::EventReady) {
    send_back_fun();
  } else {
    vtAssert(
      is_complete == AsyncEvent::EventStateType::EventWaiting,
      "Must be waiting if not ready"
    );
    /*ignore return event*/ theEvent()->attachAction(event, send_back_fun);
  }
}

/*virtual*/ AsyncEvent::~AsyncEvent() { }

void AsyncEvent::cleanup() {
  while (polling_event_container_.size() > 0) {
    testEventsTrigger();
  }
  lookup_container_.clear();
  event_container_.clear();
}

bool AsyncEvent::progress() {
  theEvent()->testEventsTrigger();
  return false;
}

bool AsyncEvent::isLocalTerm() {
  return event_container_.size() == 0;
}

NodeType AsyncEvent::getOwningNode(EventType const& event) {
  return EventManagerType::getEventNode(event);
}

bool AsyncEvent::needsPolling(EventRecordTypeType const& type) {
  return type == EventRecordTypeType::MPI_EventRecord or
         type == EventRecordTypeType::ParentEventRecord;
}

EventType AsyncEvent::createEvent(
  EventRecordTypeType const& type, NodeType const& node
) {
  EventType const event = EventManagerType::makeEvent(cur_event_, node);
  cur_event_++;

  auto et = std::make_unique<EventRecordType>(type, event);

  auto& container = needsPolling(type)
    ? polling_event_container_ : event_container_;

  container.emplace_front(EventHolderType(std::move(et)));

  lookup_container_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(event),
    std::forward_as_tuple(container.begin())
  );

  return event;
}

EventType AsyncEvent::createMPIEvent(NodeType const& node) {
  return createEvent(EventRecordTypeType::MPI_EventRecord, node);
}

EventType AsyncEvent::createNormalEvent(NodeType const& node) {
  return createEvent(EventRecordTypeType::NormalEventRecord, node);
}

EventType AsyncEvent::createParentEvent(NodeType const& node) {
  return createEvent(EventRecordTypeType::ParentEventRecord, node);
}

void AsyncEvent::removeEventID(EventType const& event) {
  lookup_container_.erase(event);
}

AsyncEvent::EventHolderType& AsyncEvent::getEventHolder(EventType const& event) {
  auto const& owning_node = getOwningNode(event);

  debug_print(
    event, node,
    "theEvent: theEventHolder: node={}, event={}, owning_node={}\n",
    theContext()->getNode(), event, owning_node
  );

  if (owning_node != theContext()->getNode()) {
    vtAssert(0, "Event does not belong to this node");
  }

  auto container_iter = lookup_container_.find(event);

  vtAssert(
    container_iter != lookup_container_.end(), "Event must exist in container"
  );

  return *container_iter->second;
}

bool AsyncEvent::holderExists(EventType const& event) {
  return lookup_container_.find(event) != lookup_container_.end();
}

AsyncEvent::EventStateType AsyncEvent::testEventComplete(EventType const& event) {
  if (holderExists(event)) {
    bool const is_ready = this->getEventHolder(event).get_event()->testReady();
    if (is_ready) {
      return EventStateType::EventReady;
    } else {
      return EventStateType::EventWaiting;
    }
  } else {
    if (getOwningNode(event) == theContext()->getNode()) {
      return EventStateType::EventReady;
    } else {
      return EventStateType::EventRemote;
    }
  }
}

void AsyncEvent::testEventsTrigger(int const& num_events) {
  int cur = 0;
  auto& cont = polling_event_container_;
  for (auto iter = cont.begin(); iter != cont.end(); ) {
    auto& holder = *iter;
    auto event = holder.get_event();
    auto id = event->getEventID();
    if (event->testReady()) {
      holder.executeActions();
      polling_event_container_.erase(iter);
      iter = lookup_container_.erase(id);
    } else {
      iter++;
    }

    cur++;
    if (num_events > 0 and cur > num_events) {
      break;
    }
  }
}

}} //end namespace vt::event
