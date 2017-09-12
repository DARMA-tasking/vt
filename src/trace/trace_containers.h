
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

using event_t = Event;
using event_class_t = EventClass;
using container_event_t = std::unordered_map<TraceEntryType, event_t>;
using container_event_class_t = std::unordered_map<TraceEntryType, event_class_t>;

// Use static template initialization pattern to deal with ordering issues with
// auto-registry
template <typename = void>
struct TraceContainers {
  static container_event_class_t event_type_container;
  static container_event_t event_container;
};

template <typename T>
container_event_class_t TraceContainers<T>::event_type_container = {};

template <typename T>
container_event_t TraceContainers<T>::event_container = {};

template <typename EventT>
struct TraceEventSeqCompare {
  bool operator()(EventT* const a, EventT* const b) const {
    return a->get_event_seq() < b->get_event_seq();
  }
};

template <typename T>
using event_compare_t = TraceEventSeqCompare<T>;

using container_event_sorted_t =
  std::map<container_event_t::mapped_type*, bool, event_compare_t<event_t>>;

using container_event_type_sorted_t =
  std::map<container_event_class_t::mapped_type*, bool, event_compare_t<event_class_t>>;

}} //end namespace runtime::trace

#endif /*__RUNTIME_TRANSPORT_TRACE_CONTAINERS__*/
