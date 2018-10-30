
#if !defined INCLUDED_TRACE_TRACE_REGISTRY_H
#define INCLUDED_TRACE_TRACE_REGISTRY_H

#include "config.h"
#include "context/context.h"

#include "trace/trace_common.h"
#include "trace/trace_event.h"
#include "trace/trace_containers.h"

namespace vt { namespace trace {

struct TraceRegistry {
  using TraceContainersType = TraceContainers<void>;

  static TraceEntryIDType registerEventHashed(
    std::string const& event_type_name, std::string const& event_name
  ) {
    // must use this old-style of print because context may not be initialized
    #if backend_check_enabled(trace_enabled) && backend_check_enabled(trace)
    fmt::print(
      "register_event_hashed: event_type_name={}, event_name={}, "
      "event_type_container.size={}\n",
      event_type_name.c_str(), event_name.c_str(),
      TraceContainersType::event_type_container.size()
    );
    #endif

    TraceEntryIDType event_type_seq = no_trace_entry_id;
    EventClassType new_event_type(event_type_name);

    auto type_iter = TraceContainersType::getEventTypeContainer().find(
      new_event_type.theEventId()
    );

    if (type_iter == TraceContainersType::getEventTypeContainer().end()) {
      event_type_seq = TraceContainersType::getEventTypeContainer().size();
      new_event_type.setEventSeq(event_type_seq);

      TraceContainersType::getEventTypeContainer().emplace(
        std::piecewise_construct,
        std::forward_as_tuple(new_event_type.theEventId()),
        std::forward_as_tuple(new_event_type)
      );
    } else {
      event_type_seq = type_iter->second.theEventSeq();
    }

    TraceEntryIDType event_seq = no_trace_entry_id;
    TraceEventType new_event(event_name, new_event_type.theEventId());

    new_event.setEventTypeSeq(event_type_seq);

    auto event_iter = TraceContainersType::getEventTypeContainer().find(
      new_event.theEventId()
    );

    if (event_iter == TraceContainersType::getEventTypeContainer().end()) {
      event_seq = TraceContainersType::getEventContainer().size();
      new_event.setEventSeq(event_seq);

      TraceContainersType::getEventContainer().emplace(
        std::piecewise_construct,
        std::forward_as_tuple(new_event.theEventId()),
        std::forward_as_tuple(new_event)
      );
    } else {
      event_seq = event_iter->second.theEventSeq();
    }

    return new_event.theEventId();
  }

};

}} //end namespace vt::trace

#endif /*INCLUDED_TRACE_TRACE_REGISTRY_H*/
