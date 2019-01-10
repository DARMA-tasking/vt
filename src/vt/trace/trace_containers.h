/*
//@HEADER
// ************************************************************************
//
//                          trace_containers.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_TRACE_TRACE_CONTAINERS_H
#define INCLUDED_TRACE_TRACE_CONTAINERS_H

#include "vt/config.h"
#include "vt/trace/trace_common.h"
#include "vt/trace/trace_event.h"

#include <cstdint>
#include <string>
#include <functional>
#include <unordered_map>
#include <map>

namespace vt { namespace trace {

template <typename T, typename U>
using EventLookupType = std::unordered_map<T, U>;

template <typename T, typename U, typename Comp>
using EventSortedType = std::map<T, U, Comp>;

using TraceEventType = Event;
using EventClassType = EventClass;
using TraceContainerEventType = EventLookupType<TraceEntryIDType, TraceEventType>;
using TraceContainerEventClassType = EventLookupType<TraceEntryIDType, EventClassType>;

struct Trace;

// Use static template initialization pattern to deal with ordering issues with
// auto-registry
template <typename = void>
class TraceContainers {
 public:
  static TraceContainerEventClassType& getEventTypeContainer(){
    return event_type_container;
  }

  static TraceContainerEventType& getEventContainer(){
    return event_container;
  }

  friend struct Trace;

 private:
  static TraceContainerEventClassType event_type_container;
  static TraceContainerEventType event_container;
};

template <typename T>
TraceContainerEventClassType TraceContainers<T>::event_type_container = {};

template <typename T>
TraceContainerEventType TraceContainers<T>::event_container = {};

template <typename EventT>
struct TraceEventSeqCompare {
  bool operator()(EventT* const a, EventT* const b) const {
    return a->theEventSeq() < b->theEventSeq();
  }
};

template <typename T>
using EventCompareType = TraceEventSeqCompare<T>;

using ContainerEventSortedType = EventSortedType<
  TraceContainerEventType::mapped_type*, bool, EventCompareType<TraceEventType>
>;

using ContainerEventTypeSortedType = EventSortedType<
  TraceContainerEventClassType::mapped_type*, bool, EventCompareType<EventClassType>
>;

}} //end namespace vt::trace

#endif /*INCLUDED_TRACE_TRACE_CONTAINERS_H*/
