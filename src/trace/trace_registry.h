
#if ! defined __RUNTIME_TRANSPORT_TRACE_REGISTRY__
#define __RUNTIME_TRANSPORT_TRACE_REGISTRY__

#include "common.h"
#include "context.h"

#include "trace_common.h"
#include "trace_event.h"
#include "trace_containers.h"

namespace runtime { namespace trace {

struct TraceRegistry {
  using TraceContainersType = TraceContainers<void>;

  static TraceEntryIDType
  register_event_hashed(
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
      new_event_type.get_event_id()
    );

    if (type_iter == TraceContainersType::event_type_container.end()) {
      event_type_seq = TraceContainersType::event_type_container.size();
      new_event_type.set_event_seq(event_type_seq);

      TraceContainersType::event_type_container.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(new_event_type.get_event_id()),
        std::forward_as_tuple(new_event_type)
      );
    } else {
      event_type_seq = type_iter->second.get_event_seq();
    }

    TraceEntryIDType event_seq = no_trace_entry_id;
    TraceEventType new_event(event_name, new_event_type.get_event_id());

    new_event.set_event_type_seq(event_type_seq);

    auto event_iter = TraceContainersType::event_container.find(
      new_event.get_event_id()
    );

    if (event_iter == TraceContainersType::event_container.end()) {
      event_seq = TraceContainersType::event_container.size();
      new_event.set_event_seq(event_seq);

      TraceContainersType::event_container.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(new_event.get_event_id()),
        std::forward_as_tuple(new_event)
      );
    } else {
      event_seq = event_iter->second.get_event_seq();
    }

    return new_event.get_event_id();
  }

};

}} //end namespace runtime::trace

#endif /*__RUNTIME_TRANSPORT_TRACE_REGISTRY__*/
