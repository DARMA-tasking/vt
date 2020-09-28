/*
//@HEADER
// *****************************************************************************
//
//                                event_record.h
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

#if !defined INCLUDED_EVENT_EVENT_RECORD_H
#define INCLUDED_EVENT_EVENT_RECORD_H

#include "vt/config.h"
#include "vt/messaging/message.h"
#include "vt/messaging/message/smart_ptr.h"
#include "vt/timing/timing_type.h"

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

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | event_list;

    s.countBytes(mpi_req);
  }
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

# if vt_check_enabled(diagnostics)
  /**
   * \internal \brief Get the creation time stamp for this record---when it was
   * constructed
   *
   * \return creation time
   */
  TimeType getCreateTime() const {
    return creation_time_stamp_;
  }
# endif

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | ready
      | msg_
      | event_union_
      | event_id_
      | type_;
  }

private:
  bool ready = false;

  MsgSharedPtr<ShortMessage> msg_ = nullptr;

  // the union for storing payload of event depending on type
  EventPayloadType event_union_;

  // the unqiue event identifier
  EventType event_id_ = no_event;

  // the type of the event record to access the union properly
  EventRecordType type_ = EventRecordType::Invalid;

# if vt_check_enabled(diagnostics)
  /// the time this event record was created
  TimeType creation_time_stamp_ = 0.;
# endif
};

}} //end namespace vt::event

#endif /*INCLUDED_EVENT_EVENT_RECORD_H*/
