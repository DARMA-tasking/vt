
#if ! defined __RUNTIME_TRANSPORT_TRACE_EVENT__
#define __RUNTIME_TRANSPORT_TRACE_EVENT__

#include "common.h"
#include "trace_common.h"

#include <cstdint>
#include <unordered_map>
#include <string>
#include <functional>

namespace runtime { namespace trace {

struct EventClass {
  EventClass(std::string const& in_event)
    : event(in_event)
  {
    auto const& event_hash =  std::hash<std::string>{}(in_event);
    this_event = event_hash;
  }

  EventClass(EventClass const&) = default;

  TraceEntryIDType
  get_event_id() const {
    return this_event;
  }

  TraceEntryIDType
  get_event_seq_id() const {
    return this_event_seq;
  }

  std::string
  get_event_name() const {
    return event;
  }

  void
  set_event_seq(TraceEntryIDType const& seq) {
    this_event_seq = seq;
  }

  TraceEntryIDType
  get_event_seq() const {
    return this_event_seq;
  }

private:
  TraceEntryIDType this_event = no_trace_entry_id;

  TraceEntryIDType this_event_seq = no_trace_entry_id;

  std::string event;
};

struct Event : EventClass {
  Event(std::string const& in_event, TraceEntryIDType const& in_event_type)
    : EventClass(in_event), this_event_type(in_event_type)
  {
    auto const& event_hash =  std::hash<std::string>{}(in_event);
    this_event_type = event_hash;
  }

  Event(Event const&) = default;

  TraceEntryIDType
  get_event_type_id() const {
    return this_event_type;
  }

  void
  set_event_type_seq(TraceEntryIDType const& seq) {
    this_event_type_seq = seq;
  }

  TraceEntryIDType
  get_event_type_seq() const {
    return this_event_type_seq;
  }

private:
  TraceEntryIDType this_event_type = no_trace_entry_id;

  TraceEntryIDType this_event_type_seq = no_trace_entry_id;
};

}} //end namespace runtime::trace

#endif /*__RUNTIME_TRANSPORT_TRACE_EVENT__*/
