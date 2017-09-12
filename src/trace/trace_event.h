
#if ! defined __RUNTIME_TRANSPORT_TRACE_EVENT__
#define __RUNTIME_TRANSPORT_TRACE_EVENT__

#include "common.h"
#include "trace_common.h"

#include <cstdint>
#include <unordered_map>
#include <string>
#include <functional>

namespace vt { namespace trace {

struct EventClass {
  EventClass(std::string const& in_event);
  EventClass(EventClass const&) = default;

  TraceEntryIDType get_event_id() const;
  TraceEntryIDType get_event_seq_id() const;

  std::string get_event_name() const;
  void set_event_seq(TraceEntryIDType const& seq);
  TraceEntryIDType get_event_seq() const;

private:
  TraceEntryIDType this_event = no_trace_entry_id;
  TraceEntryIDType this_event_seq = no_trace_entry_id;

  std::string event;
};

struct Event : EventClass {
  Event(std::string const& in_event, TraceEntryIDType const& in_event_type);
  Event(Event const&) = default;

  TraceEntryIDType get_event_type_id() const;
  void set_event_type_seq(TraceEntryIDType const& seq);
  TraceEntryIDType get_event_type_seq() const;

private:
  TraceEntryIDType this_event_type = no_trace_entry_id;
  TraceEntryIDType this_event_type_seq = no_trace_entry_id;
};

}} //end namespace vt::trace

#endif /*__RUNTIME_TRANSPORT_TRACE_EVENT__*/
