
#if !defined INCLUDED_TRACE_TRACE_EVENT_H
#define INCLUDED_TRACE_TRACE_EVENT_H

#include "vt/config.h"
#include "vt/trace/trace_common.h"

#include <cstdint>
#include <unordered_map>
#include <string>
#include <functional>

namespace vt { namespace trace {

struct EventClass {
  EventClass(std::string const& in_event);
  EventClass(EventClass const&) = default;

  TraceEntryIDType theEventId() const;
  TraceEntryIDType theEventSeqId() const;

  std::string theEventName() const;
  void setEventSeq(TraceEntryIDType const& seq);
  TraceEntryIDType theEventSeq() const;

private:
  TraceEntryIDType this_event_ = no_trace_entry_id;
  TraceEntryIDType this_event_seq_ = no_trace_entry_id;

  std::string event;
};

struct Event : EventClass {
  Event(std::string const& in_event, TraceEntryIDType const& in_event_type);
  Event(Event const&) = default;

  TraceEntryIDType theEventTypeId() const;
  void setEventTypeSeq(TraceEntryIDType const& seq);
  TraceEntryIDType theEventTypeSeq() const;

private:
  TraceEntryIDType this_event_type_ = no_trace_entry_id;
  TraceEntryIDType this_event_type_seq_ = no_trace_entry_id;
};

}} //end namespace vt::trace

#endif /*INCLUDED_TRACE_TRACE_EVENT_H*/
