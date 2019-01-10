/*
//@HEADER
// ************************************************************************
//
//                          event_id.cc
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

#include "vt/event/event_id.h"

namespace vt { namespace event {

/*static*/ EventType EventIDManager::makeEvent(
  EventIdentifierType const& id, NodeType const& node
) {
  EventType new_event_id = 0;
  EventIDManager::setEventNode(new_event_id, node);
  EventIDManager::setEventIdentifier(new_event_id, id);

  debug_print(
    event, node,
    "EventIDManager::makeEvent: id={}, node={}\n", id, node
  );

  return new_event_id;
}

/*static*/ NodeType EventIDManager::getEventNode(EventType const& event) {
  return BitPackerType::getField<
    EventIDBitsType::Node, node_num_bits, NodeType
  >(event);
}

/*static*/ void EventIDManager::setEventNode(
  EventType& event, NodeType const& node
) {
  BitPackerType::setField<EventIDBitsType::Node, node_num_bits>(event, node);
}

/*static*/ void EventIDManager::setEventIdentifier(
  EventType& event, EventIdentifierType const& id
) {
  BitPackerType::setField<
    EventIDBitsType::EventIdent, event_identifier_num_bits
  >(event, id);
}

/*static*/ EventIdentifierType EventIDManager::getEventIdentifier(
  EventType const& event
) {
  return BitPackerType::getField<
    EventIDBitsType::EventIdent, event_identifier_num_bits, EventIdentifierType
  >(event);
}

}} //end namespace vt::event
