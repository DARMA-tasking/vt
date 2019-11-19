/*
//@HEADER
// *****************************************************************************
//
//                                trace_event.cc
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


#include "vt/trace/trace_event.h"
#include "vt/utils/bits/bits_common.h"

#include <string>

namespace vt { namespace trace {

EventClass::EventClass(
  std::string const& in_event, std::string const& in_hash_event
) : event(in_event), hash_event(in_hash_event)
{
  auto const& event_hash =  std::hash<std::string>{}(in_hash_event);
  this_event_ = event_hash == 0 ? 1 : event_hash;
}

TraceEntryIDType EventClass::theEventId() const {
  return this_event_;
}

TraceEntryIDType EventClass::theEventSeqId() const {
  return this_event_seq_;
}

std::string EventClass::theEventName() const {
  return event;
}

void EventClass::setEventName(std::string const& in_str) {
  event = in_str;
}

void EventClass::setEventSeq(TraceEntryIDType const& seq) {
  this_event_seq_ = seq;
}

TraceEntryIDType EventClass::theEventSeq() const {
  return this_event_seq_;
}

Event::Event(
  std::string const& in_event, std::string const& in_hash_event,
  TraceEntryIDType const& in_event_type
) : EventClass(in_event, in_hash_event), this_event_type_(in_event_type)
{ }

TraceEntryIDType Event::theEventTypeId() const {
  return this_event_type_;
}

void Event::setEventTypeSeq(TraceEntryIDType const& seq) {
  this_event_type_seq_ = seq;
}

TraceEntryIDType Event::theEventTypeSeq() const {
  return this_event_type_seq_;
}

}} //end namespace vt::trace
