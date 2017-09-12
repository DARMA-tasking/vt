
#include "common.h"
#include "scheduler.h"
#include "active.h"
#include "event.h"
#include "termination.h"
#include "sequencer.h"

namespace vt { namespace sched {

/*static*/ void Scheduler::check_term_single_node() {
  auto const& num_nodes = theContext->get_num_nodes();
  if (num_nodes == 1) {
    theTerm->maybe_propagate();
  }
}

Scheduler::Scheduler() {
  event_triggers.resize(SchedulerEventType::SchedulerEventSize + 1);
  event_triggers_once.resize(SchedulerEventType::SchedulerEventSize + 1);
}

bool Scheduler::scheduler_impl() {
  bool scheduled_work = false;

  bool const msg_sch = theMsg->scheduler();
  bool const event_sch = theEvent->scheduler();
  bool const seq_sch = theSeq->scheduler();

  check_term_single_node();

  scheduled_work = msg_sch or event_sch or seq_sch;

  if (scheduled_work) {
    is_idle = false;
  }

  return scheduled_work;
}

void Scheduler::scheduler() {

  bool const scheduled_work1 = scheduler_impl();
  bool const scheduled_work2 = scheduler_impl();

  if (not scheduled_work1 and not scheduled_work2 and not is_idle) {
    is_idle = true;
    // idle
    trigger_event(SchedulerEventType::BeginIdle);
  }
}

void Scheduler::trigger_event(SchedulerEventType const& event) {
  assert(
    event_triggers.size() >= event and "Must be large enough to hold this event"
  );

  for (auto& t : event_triggers[event]) {
    t();
  }

  for (auto& t : event_triggers_once[event]) {
    t();
  }
  event_triggers_once[event].clear();
}

void Scheduler::register_trigger(
  SchedulerEventType const& event, TriggerType trigger
) {
  assert(
    event_triggers.size() >= event and "Must be large enough to hold this event"
  );
  event_triggers[event].push_back(trigger);
}

void Scheduler::register_trigger_once(
  SchedulerEventType const& event, TriggerType trigger
) {
  assert(
    event_triggers.size() >= event and "Must be large enough to hold this event"
  );
  event_triggers_once[event].push_back(trigger);
}

void Scheduler::scheduler_forever() {
  while (true) {
    scheduler();
  }
}

}} //end namespace vt::scheduler

namespace vt {

void run_scheduler() {
  theSched->scheduler();
}

} //end namespace vt
