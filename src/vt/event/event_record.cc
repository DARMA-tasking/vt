/*
//@HEADER
// *****************************************************************************
//
//                               event_record.cc
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

#include "vt/event/event.h"
#include "vt/event/event_record.h"
#include "vt/runtime/mpi_access.h"
#include "vt/timing/timing.h"

#include <memory>

#include <mpi.h>

namespace vt { namespace event {

EventRecord::EventRecord(EventRecordType const& type, EventType const& id)
  : event_id_(id), type_(type)
{

# if vt_check_enabled(diagnostics)
  creation_time_stamp_ = timing::Timing::getCurrentTime();
# endif

  switch (type) {
  case EventRecordType::MPI_EventRecord:
    event_union_.mpi_req = MPI_REQUEST_NULL;
    break;
  case EventRecordType::NormalEventRecord:
    break;
  case EventRecordType::ParentEventRecord:
    event_union_.event_list = new EventListType{};
    break;
  case EventRecordType::Invalid:
    vtAssert(0, "Testing readiness of invalid event record");
    break;
  default:
    vtAssert(0, "Should be impossible to reach this case");
  }

}

/*virtual*/ EventRecord::~EventRecord() {
  if (type_ == EventRecordType::ParentEventRecord) {
    delete event_union_.event_list;
  }
}

bool EventRecord::testMPIEventReady() {
  int flag = 0;
  MPI_Request* req = getRequest();
  MPI_Status stat;

  {
    VT_ALLOW_MPI_CALLS;
    MPI_Test(req, &flag, &stat);
  }

  bool const mpiready = flag == 1;

  if (mpiready and msg_ != nullptr) {
    vt_debug_print(
      verbose, event,
      "testMPIEventRead: deref: msg={}\n",
      print_ptr(msg_.get())
    );
    msg_ = nullptr;
  }

  return mpiready;
}

bool EventRecord::testNormalEventReady() {
  return false;
}

bool EventRecord::testParentEventReady() {
  bool parent_ready = true;
  auto events = getEventList();
  for (auto&& e : *events) {
    parent_ready &=
      theEvent()->testEventComplete(e) == AsyncEvent::EventStateType::EventReady;
  }
  if (parent_ready) {
    events->clear();
  }
  return parent_ready;
}

EventType EventRecord::getEventID() const {
  return event_id_;
}

void EventRecord::setManagedMessage(MsgSharedPtr<ShortMessage> in_msg) {
  msg_ = in_msg;
}

void EventRecord::setReady() {
  vtAssert(
    type_ == EventRecordType::NormalEventRecord, "Type must be normal event"
  );

  ready = true;
}

void EventRecord::addEventToList(EventType const& event) {
  vtAssert(
    type_ == EventRecordType::ParentEventRecord, "Type must be parent event"
  );

  getEventList()->push_back(event);
}

bool EventRecord::testReady() {
  switch (type_) {
  case EventRecordType::MPI_EventRecord:
    return testMPIEventReady();
  case EventRecordType::NormalEventRecord:
    return testNormalEventReady();
  case EventRecordType::ParentEventRecord:
    return testParentEventReady();
  case EventRecordType::Invalid:
    vtAssert(0, "Testing readiness of invalid event record");
    break;
  default:
    vtAssert(0, "Should be impossible to reach this case");
  }
  return false;
}

MPI_Request* EventRecord::getRequest() {
  vtAssert(
    type_ == EventRecordType::MPI_EventRecord, "Type must be MPI event"
  );

  return &event_union_.mpi_req;
}

EventListPtrType EventRecord::getEventList() const {
  vtAssert(
    type_ == EventRecordType::ParentEventRecord, "Type must be parent event"
  );

  return event_union_.event_list;
}

}} //end namespace vt::event
