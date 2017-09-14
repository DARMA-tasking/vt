
#if ! defined __RUNTIME_TRANSPORT_SCHEDULER__
#define __RUNTIME_TRANSPORT_SCHEDULER__

#include "configs/types/types_common.h"

#include <cassert>
#include <vector>
#include <list>
#include <functional>
#include <memory>

namespace vt { namespace sched {

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

  static void checkTermSingleNode();

  void scheduler();
  bool schedulerImpl();
  void schedulerForever();
  void registerTrigger(SchedulerEventType const& event, TriggerType trigger);
  void registerTriggerOnce(
    SchedulerEventType const& event, TriggerType trigger
  );
  void triggerEvent(SchedulerEventType const& event);

private:
  bool is_idle = false;

  EventTriggerContType event_triggers;
  EventTriggerContType event_triggers_once;
};

}} //end namespace vt::scheduler

namespace vt {

void runScheduler();

extern std::unique_ptr<sched::Scheduler> theSched;

}  //end namespace vt

#endif /*__RUNTIME_TRANSPORT_SCHEDULER__*/
