
#if ! defined __RUNTIME_TRANSPORT_TRACE_REGISTRY__
#define __RUNTIME_TRANSPORT_TRACE_REGISTRY__

#include "common.h"
#include "context.h"

#include "trace_common.h"
#include "trace_event.h"
#include "trace_containers.h"

namespace vt { namespace trace {

struct TraceRegistry {
  using TraceContainersType = TraceContainers<void>;

  static TraceEntryIDType registerEventHashed(
    std::string const& event_type_name, std::string const& event_name
  ) {
    // must use this old-style of print because context may not be initialized
    #if backend_check_enabled(trace_enabled) && backend_check_enabled(trace)
    printf(
      "register_event_hashed: event_type_name=%s, event_name=%s, "
      "event_type_container.size=%ld\n",
      event_type_name.c_str(), event_name.c_str(),
      TraceContainersType::event_type_container.size()
    );
    #endif

    TraceEntryIDType event_type_seq = no_trace_entry_id;
    EventClassType new_event_type(event_type_name);

    auto type_iter = TraceContainersType::event_type_container.find(
      new_event_type.getEventId()
    );

    if (type_iter == TraceContainersType::event_type_container.end()) {
      event_type_seq = TraceContainersType::event_type_container.size();
      new_event_type.setEventSeq(event_type_seq);

      TraceContainersType::event_type_container.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(new_event_type.getEventId()),
        std::forward_as_tuple(new_event_type)
      );
    } else {
      event_type_seq = type_iter->second.getEventSeq();
    }

    TraceEntryIDType event_seq = no_trace_entry_id;
    TraceEventType new_event(event_name, new_event_type.getEventId());

    new_event.setEventTypeSeq(event_type_seq);

    auto event_iter = TraceContainersType::event_container.find(
      new_event.getEventId()
    );

    if (event_iter == TraceContainersType::event_container.end()) {
      event_seq = TraceContainersType::event_container.size();
      new_event.setEventSeq(event_seq);

      TraceContainersType::event_container.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(new_event.getEventId()),
        std::forward_as_tuple(new_event)
      );
    } else {
      event_seq = event_iter->second.getEventSeq();
    }

    return new_event.getEventId();
  }

};

}} //end namespace vt::trace

#endif /*__RUNTIME_TRANSPORT_TRACE_REGISTRY__*/
