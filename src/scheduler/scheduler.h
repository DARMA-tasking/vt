
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
  using SchedulerEventType = SchedulerEvent;
  using TriggerType = std::function<void()>;
  using TriggerContainerType = std::list<TriggerType>;
  using EventTriggerContType = std::vector<TriggerContainerType>;

  Scheduler();

  static void check_term_single_node();

  void scheduler();
  bool scheduler_impl();
  void scheduler_forever();
  void register_trigger(SchedulerEventType const& event, TriggerType trigger);
  void register_trigger_once(
    SchedulerEventType const& event, TriggerType trigger
  );
  void trigger_event(SchedulerEventType const& event);

private:
  bool is_idle = false;

  EventTriggerContType event_triggers;
  EventTriggerContType event_triggers_once;
};

}} //end namespace runtime::scheduler

namespace runtime {

void run_scheduler();

extern std::unique_ptr<sched::Scheduler> the_sched;

}  //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_SCHEDULER__*/
