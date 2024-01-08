/*
//@HEADER
// *****************************************************************************
//
//                              trace_registry.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/
#include "vt/config.h"
#include "vt/context/context.h"

#include "vt/trace/trace_common.h"
#include "vt/trace/trace_event.h"
#include "vt/trace/trace_containers.h"
#include "vt/trace/trace_registry.h"

#include <functional> // std::hash

namespace vt { namespace trace {

TraceEntryIDType getEventId(std::string const& str) {
  TraceEntryIDType id = std::hash<std::string>{}(str);
  // Never allow to equal sentinel value as that violates
  // the contract in full; a very unlikely case anyway.
  // (Does not address possibility of other collisions..)
  return id != no_trace_entry_id ? id : 1;
}

/*static*/ TraceEntryIDType
TraceRegistry::registerEventHashed(
    std::string const& event_type_name, std::string const& event_name
) {
  // Trace registration (mostly) happens during initialization
  // of templates from the auto-registry.
  // This occurs BEFORE the underling 'go' flags are enabled in VT.
  // Not printing ANYTHING for the most consistent NOTHING.
  // vt_debug_print(
  //   trace, node,
  //   "register_event_hashed: event_type_name={}, event_name={}, "
  //   "event_type_count={} event_count={}\n",
  //   event_type_name.c_str(), event_name.c_str(),
  //   event_types->size(),
  //   events->size()
  // );

  TraceEntryIDType event_type_id = getEventId(event_type_name);
  TraceEntrySeqType event_type_seq = no_trace_entry_seq;

  { // ensure event type / category
    auto* event_types = TraceContainers::getEventTypeContainer();

    auto type_iter = event_types->find(event_type_id);
    if (type_iter == event_types->end()) {
      event_type_seq = event_types->size();

      event_types->insert({
        event_type_id,
        EventClassType{event_type_id, event_type_seq, event_type_name}
      });
    } else {
      event_type_seq = type_iter->second.theEventSeq();
    }
  }

  { // ensure event
    auto* events = TraceContainers::getEventContainer();

    TraceEntryIDType event_id = getEventId(
      event_type_name + std::string("::") + event_name
    );

    auto event_iter = events->find(event_id);

    if (event_iter == events->end()) {
      TraceEntrySeqType event_seq = events->size();

      events->insert({
        event_id,
        TraceEventType{event_id, event_seq, event_name, event_type_id, event_type_seq}
      });
    }

    // found or newly added
    return event_id;
  }
}

/*static*/ void
TraceRegistry::setTraceName(
  TraceEntryIDType id, std::string const& name, std::string const& type_name
) {
#if vt_check_enabled(trace_enabled)
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

static EventClassType not_found_ = EventClassType{
  no_trace_entry_id, no_trace_entry_seq, std::string{}
};

/*static*/ EventClassType const&
TraceRegistry::getEvent(TraceEntryIDType id) {
  auto* events = TraceContainers::getEventContainer();
  auto iter = events->find(id);
  if (iter != events->end()) {
    return iter->second;
  }
  return not_found_;
}

}} //end namespace vt::trace

