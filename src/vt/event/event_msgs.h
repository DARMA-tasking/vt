/*
//@HEADER
// *****************************************************************************
//
//                                 event_msgs.h
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

#if !defined INCLUDED_EVENT_EVENT_MSGS_H
#define INCLUDED_EVENT_EVENT_MSGS_H

#include "vt/config.h"
#include "vt/messaging/message.h"

namespace vt {

struct EventCheckFinishedMsg : ShortMessage {
  EventType event_ = 0, event_back_ = 0;
  NodeType sent_from_node_ = 0;

  EventCheckFinishedMsg(
    EventType const& event_in, NodeType const& in_sent_from_node,
    EventType const& event_back_in
  )
    : ShortMessage(), event_(event_in), event_back_(event_back_in),
      sent_from_node_(in_sent_from_node)
  { }
};

struct EventFinishedMsg : ShortMessage {
  EventType event_ = 0;
  EventType event_back_ = 0;

  EventFinishedMsg(EventType const& event_in, EventType const& event_back_in)
    : ShortMessage(), event_(event_in), event_back_(event_back_in)
  { }
};

extern HandlerType event_finished_han;
extern HandlerType check_event_finished_han;

} //end namespace vt

#endif /*INCLUDED_EVENT_EVENT_MSGS_H*/
