
#if ! defined __RUNTIME_TRANSPORT_TRACE_EVENT__
#define __RUNTIME_TRANSPORT_TRACE_EVENT__

#include "config.h"
#include "trace_common.h"

#include <cstdint>
#include <unordered_map>
#include <string>
#include <functional>

namespace vt { namespace trace {

struct EventClass {
  EventClass(std::string const& in_event);
  EventClass(EventClass const&) = default;

  TraceEntryIDType getEventId() const;
  TraceEntryIDType getEventSeqId() const;

  std::string getEventName() const;
  void setEventSeq(TraceEntryIDType const& seq);
  TraceEntryIDType getEventSeq() const;

private:
  TraceEntryIDType this_event_ = no_trace_entry_id;
  TraceEntryIDType this_event_seq_ = no_trace_entry_id;

  std::string event;
};

struct Event : EventClass {
  Event(std::string const& in_event, TraceEntryIDType const& in_event_type);
  Event(Event const&) = default;

  TraceEntryIDType getEventTypeId() const;
  void setEventTypeSeq(TraceEntryIDType const& seq);
  TraceEntryIDType getEventTypeSeq() const;

private:
  TraceEntryIDType this_event_type_ = no_trace_entry_id;
  TraceEntryIDType this_event_type_seq_ = no_trace_entry_id;
};

}} //end namespace vt::trace

#endif /*__RUNTIME_TRANSPORT_TRACE_EVENT__*/
