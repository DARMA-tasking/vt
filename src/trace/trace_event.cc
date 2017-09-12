
#include "trace_event.h"

#include <string>

namespace vt { namespace trace {

EventClass::EventClass(std::string const& in_event)
  : event(in_event)
{
  auto const& event_hash =  std::hash<std::string>{}(in_event);
  this_event = event_hash;
}

TraceEntryIDType EventClass::get_event_id() const {
  return this_event;
}

TraceEntryIDType EventClass::get_event_seq_id() const {
  return this_event_seq;
}

std::string EventClass::get_event_name() const {
  return event;
}

void EventClass::set_event_seq(TraceEntryIDType const& seq) {
  this_event_seq = seq;
}

TraceEntryIDType EventClass::get_event_seq() const {
  return this_event_seq;
}

Event::Event(std::string const& in_event, TraceEntryIDType const& in_event_type)
  : EventClass(in_event), this_event_type(in_event_type)
{
  auto const& event_hash =  std::hash<std::string>{}(in_event);
  this_event_type = event_hash;
}

TraceEntryIDType Event::get_event_type_id() const {
  return this_event_type;
}

void Event::set_event_type_seq(TraceEntryIDType const& seq) {
  this_event_type_seq = seq;
}

TraceEntryIDType Event::get_event_type_seq() const {
  return this_event_type_seq;
}

}} //end namespace vt::trace
