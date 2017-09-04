
#include "common.h"
#include "scheduler.h"
#include "active.h"
#include "event.h"
#include "termination.h"
#include "sequencer.h"

namespace runtime { namespace scheduler {

/*static*/ void
Scheduler::check_term_single_node() {
  auto const& num_nodes = the_context->get_num_nodes();
  if (num_nodes == 1) {
    the_term->maybe_propagate();
  }
}

/*static*/ void
Scheduler::scheduler() {
  bool scheduled_work = false;

  scheduled_work = scheduled_work or the_msg->scheduler();
  scheduled_work = scheduled_work or the_seq->scheduler();
  scheduled_work = scheduled_work or the_event->scheduler();
  check_term_single_node();

  if (not scheduled_work) {
    // idle
  }
}

/*static*/ void
Scheduler::scheduler_forever() {
  while (true) {
    scheduler();
  }
}

}} //end namespace runtime::scheduler

namespace runtime {

void
run_scheduler() {
  scheduler::Scheduler::scheduler();
}

} //end namespace runtime
