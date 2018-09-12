
#include "event.h"
#include "event_record.h"

#include <memory>

#include <mpi.h>

namespace vt { namespace event {

EventRecord::EventRecord(EventRecordType const& type, EventType const& id)
  : event_id_(id), type_(type) {

  switch (type) {
  case EventRecordType::MPI_EventRecord:
    event_union_.mpi_req = MPI_Request();
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

  MPI_Test(req, &flag, &stat);

  bool const ready = flag == 1;

  if (ready and msg_ != nullptr and isSharedMessage(msg_.get())) {
    debug_print(
      verbose, active, node,
      "testMPIEventRead: deref: msg={}\n",
      print_ptr(msg_.get())
    );
    msg_ = nullptr;
  }

  return ready;
}

bool EventRecord::testNormalEventReady() {
  return false;
}

bool EventRecord::testParentEventReady() {
  bool ready = true;
  auto events = getEventList();
  for (auto&& e : *events) {
    ready &=
      theEvent()->testEventComplete(e) == AsyncEvent::EventStateType::EventReady;
  }
  if (ready) {
    events->clear();
  }
  return ready;
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
    break;
  case EventRecordType::NormalEventRecord:
    return testNormalEventReady();
    break;
  case EventRecordType::ParentEventRecord:
    return testParentEventReady();
    break;
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
