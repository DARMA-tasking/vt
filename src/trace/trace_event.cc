
#include "trace_event.h"

#include <string>

namespace vt { namespace trace {

EventClass::EventClass(std::string const& in_event)
  : event(in_event)
{
  auto const& event_hash =  std::hash<std::string>{}(in_event);
  this_event_ = event_hash;
}

TraceEntryIDType EventClass::getEventId() const {
  return this_event_;
}

TraceEntryIDType EventClass::getEventSeqId() const {
  return this_event_seq_;
}

std::string EventClass::getEventName() const {
  return event;
}

void EventClass::setEventSeq(TraceEntryIDType const& seq) {
  this_event_seq_ = seq;
}

TraceEntryIDType EventClass::getEventSeq() const {
  return this_event_seq_;
}

Event::Event(std::string const& in_event, TraceEntryIDType const& in_event_type)
  : EventClass(in_event), this_event_type_(in_event_type)
{
  auto const& event_hash =  std::hash<std::string>{}(in_event);
  this_event_type_ = event_hash;
}

TraceEntryIDType Event::getEventTypeId() const {
  return this_event_type_;
}

void Event::setEventTypeSeq(TraceEntryIDType const& seq) {
  this_event_type_seq_ = seq;
}

TraceEntryIDType Event::getEventTypeSeq() const {
  return this_event_type_seq_;
}

}} //end namespace vt::trace
