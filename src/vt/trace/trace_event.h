/*
//@HEADER
// *****************************************************************************
//
//                                trace_event.h
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
  void setEventName(std::string const& in_str);
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
