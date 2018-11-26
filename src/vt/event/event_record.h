/*
//@HEADER
// ************************************************************************
//
//                          event_record.h
//                                VT
//              Copyright (C) 2017 NTESS, LLC
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

#if !defined INCLUDED_EVENT_EVENT_RECORD_H
#define INCLUDED_EVENT_EVENT_RECORD_H

#include "vt/config.h"
#include "vt/messaging/message.h"
#include "vt/messaging/message/smart_ptr.h"

#include <memory>
#include <vector>

#include <mpi.h>

namespace vt { namespace event {

enum struct eEventRecord : int8_t {
  MPI_EventRecord = 1,
  ParentEventRecord = 2,
  NormalEventRecord = 3,
  Invalid = -1
};

using EventListType = std::vector<EventType>;
using EventListPtrType = EventListType*;

union uEventPayload {
  MPI_Request mpi_req;
  EventListPtrType event_list;
};

struct EventRecord {
  using EventRecordType = eEventRecord;
  using EventPayloadType = uEventPayload;

  EventRecord() = default;

  virtual ~EventRecord();

  EventRecord(EventRecordType const& type, EventType const& id);

  bool testMPIEventReady();
  bool testNormalEventReady();
  bool testParentEventReady();
  void addEventToList(EventType const& event);
  EventType getEventID() const;
  bool testReady();
  void setReady();
  MPI_Request* getRequest();
  EventListPtrType getEventList() const;
  void setManagedMessage(MsgSharedPtr<ShortMessage> in_msg);

private:
  bool ready = false;

  MsgSharedPtr<ShortMessage> msg_ = nullptr;

  // the union for storing payload of event depending on type
  EventPayloadType event_union_;

  // the unqiue event identifier
  EventType event_id_ = no_event;

  // the type of the event record to access the union properly
  EventRecordType type_ = EventRecordType::Invalid;
};

}} //end namespace vt::event

#endif /*INCLUDED_EVENT_EVENT_RECORD_H*/
