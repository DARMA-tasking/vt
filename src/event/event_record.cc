
#include <memory>

#include "event.h"
#include "event_record.h"

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
    assert(0 and "Testing readiness of invalid event record");
    break;
  default:
    assert(0 and "Should be impossible to reach this case");
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

  if (ready and msg_ != nullptr and isSharedMessage(msg_)) {
    messageDeref(msg_);
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
      theEvent->testEventComplete(e) == AsyncEvent::EventStateType::EventReady;
  }
  if (ready) {
    events->clear();
  }
  return ready;
}

EventType EventRecord::getEventID() const {
  return event_id_;
}

void EventRecord::setManagedMessage(ShortMessage* in_msg) {
  msg_ = in_msg;
  messageRef(msg_);
}

void EventRecord::setReady() {
  assert(
    type_ == EventRecordType::NormalEventRecord and "Type must be normal event"
  );

  ready = true;
}

void EventRecord::addEventToList(EventType const& event) {
  assert(
    type_ == EventRecordType::ParentEventRecord and "Type must be parent event"
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
    assert(0 and "Testing readiness of invalid event record");
    break;
  default:
    assert(0 and "Should be impossible to reach this case");
  }
  return false;
}

MPI_Request* EventRecord::getRequest() {
  assert(
    type_ == EventRecordType::MPI_EventRecord and "Type must be MPI event"
  );

  return &event_union_.mpi_req;
}

EventListPtrType EventRecord::getEventList() const {
  assert(
    type_ == EventRecordType::ParentEventRecord and "Type must be parent event"
  );

  return event_union_.event_list;
}

}} //end namespace vt::event
