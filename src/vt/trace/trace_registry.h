/*
//@HEADER
// *****************************************************************************
//
//                               trace_registry.h
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_TRACE_TRACE_REGISTRY_H
#define INCLUDED_TRACE_TRACE_REGISTRY_H

#include "vt/config.h"
#include "vt/context/context.h"

#include "vt/trace/trace_common.h"
#include "vt/trace/trace_event.h"
#include "vt/trace/trace_containers.h"

namespace vt { namespace trace {

struct TraceRegistry {
  using TraceContainersType = TraceContainers<void>;

  static TraceEntryIDType registerEventHashed(
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
    EventClassType new_event_type(event_type_name);

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
    TraceEventType new_event(event_name, new_event_type.theEventId());

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

};

}} //end namespace vt::trace

#endif /*INCLUDED_TRACE_TRACE_REGISTRY_H*/
