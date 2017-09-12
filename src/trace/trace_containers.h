
#if ! defined __RUNTIME_TRANSPORT_TRACE_CONTAINERS__
#define __RUNTIME_TRANSPORT_TRACE_CONTAINERS__

#include "common.h"
#include "trace_common.h"
#include "trace_event.h"

#include <cstdint>
#include <string>
#include <functional>
#include <unordered_map>
#include <map>

namespace runtime { namespace trace {

using TraceEventType = Event;
using EventClassType = EventClass;
using TraceContainerEventType = std::unordered_map<TraceEntryIDType, TraceEventType>;
using TraceContainerEventClassType = std::unordered_map<TraceEntryIDType, EventClassType>;

// Use static template initialization pattern to deal with ordering issues with
// auto-registry
template <typename = void>
struct TraceContainers {
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
    return a->get_event_seq() < b->get_event_seq();
  }
};

template <typename T>
using event_compare_t = TraceEventSeqCompare<T>;

using container_event_sorted_t =
  std::map<TraceContainerEventType::mapped_type*, bool, event_compare_t<TraceEventType>>;

using container_event_type_sorted_t =
  std::map<TraceContainerEventClassType::mapped_type*, bool, event_compare_t<EventClassType>>;

}} //end namespace runtime::trace

#endif /*__RUNTIME_TRANSPORT_TRACE_CONTAINERS__*/
