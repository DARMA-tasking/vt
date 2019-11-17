#include "vt/config.h"
#include "vt/context/context.h"

#include "vt/trace/trace_common.h"
#include "vt/trace/trace_event.h"
#include "vt/trace/trace_containers.h"
#include "vt/trace/trace_registry.h"

namespace vt { namespace trace {

/*static*/ TraceEntryIDType
TraceRegistry::registerEventHashed(
    std::string const& event_type_name, std::string const& event_name
  ) {
  #if backend_check_enabled(trace_enabled) && backend_check_enabled(trace)
  debug_print(
    trace, node,
    "register_event_hashed: event_type_name={}, event_name={}, "
    "event_type_container.size={}\n",
    event_type_name.c_str(), event_name.c_str(),
    TraceContainersType::getEventTypeContainer().size()
  );
  #endif

  TraceEntryIDType event_type_seq = no_trace_entry_id;
  EventClassType new_event_type(event_type_name, event_type_name);

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
  TraceEventType new_event(
    event_name,
    event_type_name + std::string("::") + event_name,
    new_event_type.theEventId()
  );

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

}} //end namespace vt::trace

