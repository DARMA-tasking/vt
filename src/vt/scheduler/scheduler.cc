
#include "vt/config.h"
#include "vt/scheduler/scheduler.h"
#include "vt/messaging/active.h"
#include "vt/event/event.h"
#include "vt/termination/termination.h"
#include "vt/runtime/runtime_get.h"

namespace vt { namespace sched {

/*static*/ void Scheduler::checkTermSingleNode() {
  auto const& num_nodes = theContext()->getNumNodes();
  if (num_nodes == 1) {
    theTerm()->maybePropagate();
  }
}

Scheduler::Scheduler() {
  event_triggers.resize(SchedulerEventType::SchedulerEventSize + 1);
  event_triggers_once.resize(SchedulerEventType::SchedulerEventSize + 1);
}

bool Scheduler::schedulerImpl() {
  bool scheduled_work = false;

  bool const msg_sch = theMsg()->scheduler();
  bool const event_sch = theEvent()->scheduler();
  bool const seq_sch = theSeq()->scheduler();
  bool const vrt_seq_sch = theVirtualSeq()->scheduler();
  bool const collection_sch = theCollection()->scheduler<>();
  bool const worker_sch =
    theContext()->hasWorkers() ? theWorkerGrp()->progress(),false : false;
  bool const worker_comm_sch =
    theContext()->hasWorkers() ? theWorkerGrp()->commScheduler() : false;

  checkTermSingleNode();

  scheduled_work =
    msg_sch or event_sch or seq_sch or vrt_seq_sch or
    worker_sch or worker_comm_sch or collection_sch;

  if (scheduled_work) {
    is_idle = false;
  }

  return scheduled_work;
}

void Scheduler::scheduler() {
  bool const scheduled_work1 = schedulerImpl();
  bool const scheduled_work2 = schedulerImpl();

  if (not scheduled_work1 and not scheduled_work2 and not is_idle) {
    is_idle = true;
    // idle
    triggerEvent(SchedulerEventType::BeginIdle);
  }

  has_executed_ = true;
}

void Scheduler::triggerEvent(SchedulerEventType const& event) {
  vtAssert(
    event_triggers.size() >= event, "Must be large enough to hold this event"
  );

  for (auto& t : event_triggers[event]) {
    t();
  }

  for (auto& t : event_triggers_once[event]) {
    t();
  }
  event_triggers_once[event].clear();
}

void Scheduler::registerTrigger(
  SchedulerEventType const& event, TriggerType trigger
) {
  vtAssert(
    event_triggers.size() >= event, "Must be large enough to hold this event"
  );
  event_triggers[event].push_back(trigger);
}

void Scheduler::registerTriggerOnce(
  SchedulerEventType const& event, TriggerType trigger
) {
  vtAssert(
    event_triggers.size() >= event, "Must be large enough to hold this event"
  );
  event_triggers_once[event].push_back(trigger);
}

void Scheduler::schedulerForever() {
  while (true) {
    scheduler();
  }
}

}} //end namespace vt::scheduler

namespace vt {

void runScheduler() {
  theSched()->scheduler();
}

} //end namespace vt
