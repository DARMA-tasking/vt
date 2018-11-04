
#if !defined INCLUDED_TRACE_TRACE_CONTAINERS_H
#define INCLUDED_TRACE_TRACE_CONTAINERS_H

#include "vt/config.h"
#include "vt/trace/trace_common.h"
#include "vt/trace/trace_event.h"

#include <cstdint>
#include <string>
#include <functional>
#include <unordered_map>
#include <map>

namespace vt { namespace trace {

template <typename T, typename U>
using EventLookupType = std::unordered_map<T, U>;

template <typename T, typename U, typename Comp>
using EventSortedType = std::map<T, U, Comp>;

using TraceEventType = Event;
using EventClassType = EventClass;
using TraceContainerEventType = EventLookupType<TraceEntryIDType, TraceEventType>;
using TraceContainerEventClassType = EventLookupType<TraceEntryIDType, EventClassType>;

// Use static template initialization pattern to deal with ordering issues with
// auto-registry
template <typename = void>
class TraceContainers {
 public:
  static TraceContainerEventClassType& getEventTypeContainer(){
    return event_type_container;
  }

  static TraceContainerEventType& getEventContainer(){
    return event_container;
  }

 private:
  static TraceContainerEventClassType event_type_container;
  static TraceContainerEventType event_container;
};

template <typename T>
TraceContainerEventClassType TraceContainers<T>::event_type_container = {};

template <typename T>
TraceContainerEventType TraceContainers<T>::event_container = {};

template <typename EventT>
struct TraceEventSeqCompare {
  bool operator()(EventT* const a, EventT* const b) const {
    return a->theEventSeq() < b->theEventSeq();
  }
};

template <typename T>
using EventCompareType = TraceEventSeqCompare<T>;

using ContainerEventSortedType = EventSortedType<
  TraceContainerEventType::mapped_type*, bool, EventCompareType<TraceEventType>
>;

using ContainerEventTypeSortedType = EventSortedType<
  TraceContainerEventClassType::mapped_type*, bool, EventCompareType<EventClassType>
>;

}} //end namespace vt::trace

#endif /*INCLUDED_TRACE_TRACE_CONTAINERS_H*/
