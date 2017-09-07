
#if ! defined __RUNTIME_TRANSPORT_SCHEDULER__
#define __RUNTIME_TRANSPORT_SCHEDULER__

#include "common.h"

#include <cassert>
#include <vector>
#include <list>
#include <functional>
#include <memory>

namespace runtime { namespace sched {

enum SchedulerEvent {
  BeginIdle = 0,
  EndIdle = 1,
  SchedulerEventSize = 2
};

struct Scheduler {
  using scheduler_event_t = SchedulerEvent;
  using trigger_t = std::function<void()>;
  using trigger_container_t = std::list<trigger_t>;
  using event_trigger_cont_t = std::vector<trigger_container_t>;

  Scheduler();

  static void
  check_term_single_node();

  void
  scheduler();

  bool
  scheduler_impl();

  void
  scheduler_forever();

  void
  register_trigger(scheduler_event_t const& event, trigger_t trigger);

  void
  register_trigger_once(scheduler_event_t const& event, trigger_t trigger);

  void
  trigger_event(scheduler_event_t const& event);

private:
  bool is_idle = false;

  event_trigger_cont_t event_triggers;

  event_trigger_cont_t event_triggers_once;
};

}} //end namespace runtime::scheduler

namespace runtime {

void
run_scheduler();

extern std::unique_ptr<sched::Scheduler> the_sched;

}  //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_SCHEDULER__*/
