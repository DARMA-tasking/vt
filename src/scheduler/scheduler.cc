
#include "common.h"
#include "scheduler.h"
#include "active.h"
#include "event.h"
#include "termination.h"
#include "sequencer.h"

namespace runtime { namespace sched {

/*static*/ void
Scheduler::check_term_single_node() {
  auto const& num_nodes = the_context->get_num_nodes();
  if (num_nodes == 1) {
    the_term->maybe_propagate();
  }
}

Scheduler::Scheduler() {
  event_triggers.resize(scheduler_event_t::SchedulerEventSize + 1);
}

bool
Scheduler::scheduler_impl() {
  bool scheduled_work = false;

  bool const msg_sch = the_msg->scheduler();
  bool const event_sch = the_event->scheduler();
  bool const seq_sch = the_seq->scheduler();

  check_term_single_node();

  scheduled_work = msg_sch or event_sch or seq_sch;

  if (scheduled_work) {
    is_idle = false;
  }

  return scheduled_work;
}

void
Scheduler::scheduler() {

  bool const scheduled_work1 = scheduler_impl();
  bool const scheduled_work2 = scheduler_impl();

  if (not scheduled_work1 and not scheduled_work2 and not is_idle) {
    is_idle = true;
    // idle
    trigger_event(scheduler_event_t::BeginIdle);
  }
}

void
Scheduler::trigger_event(scheduler_event_t const& event) {
  assert(
    event_triggers.size() >= event and "Must be large enough to hold this event"
  );

  for (auto& t : event_triggers[event]) {
    t();
  }
}

void
Scheduler::register_trigger(
  scheduler_event_t const& event, trigger_t trigger
) {
  assert(
    event_triggers.size() >= event and "Must be large enough to hold this event"
  );
  event_triggers[event].push_back(trigger);
}

void
Scheduler::scheduler_forever() {
  while (true) {
    scheduler();
  }
}

}} //end namespace runtime::scheduler

namespace runtime {

void
run_scheduler() {
  the_sched->scheduler();
}

} //end namespace runtime
