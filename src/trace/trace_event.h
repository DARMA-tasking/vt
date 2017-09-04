
#if ! defined __RUNTIME_TRANSPORT_TRACE_EVENT__
#define __RUNTIME_TRANSPORT_TRACE_EVENT__

#include "common.h"
#include "trace_common.h"

#include <cstdint>
#include <unordered_map>
#include <string>
#include <functional>

namespace runtime { namespace trace {

struct EventType {
  EventType(std::string const& in_event)
    : event(in_event)
  {
    auto const& event_hash =  std::hash<std::string>{}(in_event);
    this_event = event_hash;
  }

  EventType(EventType const&) = default;

  trace_ep_t
  get_event_id() const {
    return this_event;
  }

  trace_ep_t
  get_event_seq_id() const {
    return this_event_seq;
  }

  std::string
  get_event_name() const {
    return event;
  }

  void
  set_event_seq(trace_ep_t const& seq) {
    this_event_seq = seq;
  }

  trace_ep_t
  get_event_seq() const {
    return this_event_seq;
  }

private:
  trace_ep_t this_event = no_trace_ep;

  trace_ep_t this_event_seq = no_trace_ep;

  std::string event;
};

struct Event : EventType {
  Event(std::string const& in_event, trace_ep_t const& in_event_type)
    : EventType(in_event), this_event_type(in_event_type)
  {
    auto const& event_hash =  std::hash<std::string>{}(in_event);
    this_event_type = event_hash;
  }

  Event(Event const&) = default;

  trace_ep_t
  get_event_type_id() const {
    return this_event_type;
  }

  void
  set_event_type_seq(trace_ep_t const& seq) {
    this_event_type_seq = seq;
  }

  trace_ep_t
  get_event_type_seq() const {
    return this_event_type_seq;
  }

private:
  trace_ep_t this_event_type = no_trace_ep;

  trace_ep_t this_event_type_seq = no_trace_ep;
};

}} //end namespace runtime::trace

#endif /*__RUNTIME_TRANSPORT_TRACE_EVENT__*/
