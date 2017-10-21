
#include "config.h"
#include "event.h"
#include "event_holder.h"

namespace vt { namespace event {

EventHolder::EventRecordType* EventHolder::get_event() const {
  return event_.get();
}

void EventHolder::attachAction(ActionType action) {
  actions_.emplace_back(action);
}

void EventHolder::makeReadyTrigger() {
  //printf("make_ready_trigger\n");
  event_->setReady();
  executeActions();
  theEvent()->removeEventID(event_->getEventID());
}

void EventHolder::executeActions() {
  for (auto&& action : actions_) {
    action();
  }
  actions_.clear();
}

}} //end namespace vt::event
