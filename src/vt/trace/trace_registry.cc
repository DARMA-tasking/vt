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
  auto* event_types = TraceContainers::getEventTypeContainer();
  auto* events = TraceContainers::getEventContainer();

  // Trace registration (mostly) happens during initialization
  // of templates from the auto-registy.
  // This occurs BEFORE the underling 'go' flags are enabled in VT.
  // Not printing ANYTHING for the most consistent NOTHING.
  // debug_print(
  //   trace, node,
  //   "register_event_hashed: event_type_name={}, event_name={}, "
  //   "event_type_count={} event_count={}\n",
  //   event_type_name.c_str(), event_name.c_str(),
  //   event_types->size(),
  //   events->size()
  // );

  TraceEntryIDType event_type_seq = no_trace_entry_id;
  EventClassType new_event_type(event_type_name, event_type_name);

  auto type_iter = event_types->find(
    new_event_type.theEventId()
  );

  if (type_iter == event_types->end()) {
    event_type_seq = event_types->size();
    new_event_type.setEventSeq(event_type_seq);

    event_types->emplace(
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

  auto event_iter = events->find(
    new_event.theEventId()
  );

  if (event_iter == events->end()) {
    event_seq = events->size();
    new_event.setEventSeq(event_seq);

    events->emplace(
      std::piecewise_construct,
      std::forward_as_tuple(new_event.theEventId()),
      std::forward_as_tuple(new_event)
    );
  } else {
    event_seq = event_iter->second.theEventSeq();
  }

  return new_event.theEventId();
}

/*static*/ void
TraceRegistry::setTraceName(
  TraceEntryIDType id, std::string const& name, std::string const& type_name
) {
#if backend_check_enabled(trace_enabled)
  auto* events = TraceContainers::getEventContainer();
  auto event_iter = events->find(id);
  // TODO, increase guard here perhaps:
  // vtAssertInfo(
  //   iter != event_types->end(),
  //   "Event must exist",
  //   name, parent, id, type_id
  // );

  if (event_iter != events->end()) {
    auto type_id = event_iter->second.theEventTypeId();
    if (name != "") {
      event_iter->second.setEventName(name);
    }

    if (type_name != "") {
      auto* event_types = TraceContainers::getEventTypeContainer();
      auto iter = event_types->find(type_id);
      vtAssertInfo(
        iter != event_types->end(),
        "Event type must exist",
        name, type_name, id, type_id
      );
      if (iter != event_types->end()) {
        iter->second.setEventName(type_name);
      }
    }
  }
#endif
}

/*static*/ bool
TraceRegistry::getEventSequence(TraceEntryIDType id, TraceEntryIDType &seq) {
  auto* events = TraceContainers::getEventContainer();
  auto iter = events->find(id);
  if (iter != events->end()) {
    seq = iter->second.theEventSeqId();
    return true;
  }
  return false;
}

}} //end namespace vt::trace

