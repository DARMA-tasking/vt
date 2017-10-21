
#if !defined INCLUDED_EVENT_EVENT_HOLDER_H
#define INCLUDED_EVENT_EVENT_HOLDER_H

#include <memory>
#include <vector>

#include "config.h"
#include "event_record.h"

namespace vt { namespace event {

struct EventHolder {
  using EventRecordType = EventRecord;
  using EventRecordPtrType = std::unique_ptr<EventRecordType>;
  using ActionContainerType = std::vector<ActionType>;

  EventHolder() = default;

  explicit EventHolder(EventRecordPtrType in_event)
    : event_(std::move(in_event))
  { }

  EventRecordType* get_event() const;
  void attachAction(ActionType action);
  void makeReadyTrigger();
  void executeActions();

private:
  EventRecordPtrType event_;

  // actions to trigger when this event completes
  ActionContainerType actions_;
};

}} //end namespace vt::event

#endif /*INCLUDED_EVENT_EVENT_HOLDER_H*/
