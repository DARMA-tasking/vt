
#if ! defined __RUNTIME_TRANSPORT_EVENT_RECORD__
#define __RUNTIME_TRANSPORT_EVENT_RECORD__

#include <memory>
#include <vector>

#include <mpi.h>

#include "config.h"
#include "message.h"

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
  void setManagedMessage(ShortMessage* in_msg);

private:
  bool ready = false;

  ShortMessage* msg_ = nullptr;

  // the union for storing payload of event depending on type
  EventPayloadType event_union_;

  // the unqiue event identifier
  EventType event_id_ = no_event;

  // the type of the event record to access the union properly
  EventRecordType type_ = EventRecordType::Invalid;
};

}} //end namespace vt::event

#endif /*__RUNTIME_TRANSPORT_EVENT_RECORD__*/
