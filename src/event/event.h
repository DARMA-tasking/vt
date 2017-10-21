
#if !defined INCLUDED_EVENT_EVENT_H
#define INCLUDED_EVENT_EVENT_H

#include <memory>
#include <vector>
#include <list>
#include <functional>
#include <unordered_map>

#include <mpi.h>

#include "config.h"
#include "context/context.h"
#include "event_record.h"
#include "event_id.h"
#include "event_holder.h"
#include "event_msgs.h"

namespace vt { namespace event {

enum class EventState : int8_t {
  EventReady = 1,
  EventWaiting = 2,
  EventRemote = 3
};

struct AsyncEvent {
  using EventRecordTypeType = eEventRecord;
  using EventManagerType = EventIDManager;
  using EventStateType = EventState;
  using EventRecordType = EventRecord;
  using EventRecordPtrType = std::unique_ptr<EventRecordType>;
  using EventHolderType = EventHolder;
  using EventHolderPtrType = EventHolder*;
  using TypedEventContainerType = std::list<EventHolderType>;
  using EventContIter = typename TypedEventContainerType::iterator;
  using EventContainerType = std::unordered_map<EventType, EventContIter>;

  AsyncEvent() = default;

  virtual ~AsyncEvent();

  void cleanup();
  EventType createEvent(EventRecordTypeType const& type, NodeType const& node);
  EventRecordType& getEvent(EventType const& event);
  NodeType getOwningNode(EventType const& event);
  EventType createMPIEvent(NodeType const& node);
  EventType createNormalEvent(NodeType const& node);
  EventType createParentEvent(NodeType const& node);
  EventHolderType& getEventHolder(EventType const& event);
  bool holderExists(EventType const& event);
  bool needsPolling(EventRecordTypeType const& type);
  void removeEventID(EventType const& event);
  EventStateType testEventComplete(EventType const& event);
  EventType attachAction(EventType const& event, ActionType callable);
  void testEventsTrigger(int const& num_events = num_check_actions);
  bool scheduler();
  bool isLocalTerm();

  static void eventFinished(EventFinishedMsg* msg);
  static void checkEventFinished(EventCheckFinishedMsg* msg);

private:
  // next event id
  EventType cur_event_ = 0;

  // std::list of events
  TypedEventContainerType event_container_;

  // list of events that need polling for progress
  TypedEventContainerType polling_event_container_;

  // container to lookup events by EventType
  EventContainerType lookup_container_;
};

}} //end namespace vt::event

namespace vt {

using EventRecordType = event::EventRecord;

extern event::AsyncEvent* theEvent();

} //end namespace vt

#endif /*INCLUDED_EVENT_EVENT_H*/
