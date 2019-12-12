/*
//@HEADER
// *****************************************************************************
//
//                              trace_containers.h
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

#if !defined INCLUDED_TRACE_TRACE_CONTAINERS_H
#define INCLUDED_TRACE_TRACE_CONTAINERS_H

#include "vt/config.h"
#include "vt/trace/trace_common.h"
#include "vt/trace/trace_event.h"

#include <unordered_map>

namespace vt { namespace trace {

using TraceEventType = Event;
using EventClassType = EventClass;
using TraceContainerEventType = std::unordered_map<TraceEntryIDType, TraceEventType>;
using TraceContainerEventClassType = std::unordered_map<TraceEntryIDType, EventClassType>;

struct TraceContainers {
  static TraceContainerEventClassType& getEventTypeContainer(){
    return event_type_container_;
  }

  static TraceContainerEventType& getEventContainer(){
    return event_container_;
  }

private:
  static TraceContainerEventClassType event_type_container_;
  static TraceContainerEventType event_container_;
};

}} //end namespace vt::trace

#endif /*INCLUDED_TRACE_TRACE_CONTAINERS_H*/
