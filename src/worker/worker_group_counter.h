
#if !defined INCLUDED_WORKER_WORKER_GROUP_COUNTER_H
#define INCLUDED_WORKER_WORKER_GROUP_COUNTER_H

#include "config.h"
#include "worker/worker_common.h"
#include "utils/atomic/atomic.h"

#include <list>

namespace vt { namespace worker {

using ::vt::util::atomic::AtomicType;

struct WorkerGroupCounter {
  using IdleListenerType = std::function<void(eWorkerGroupEvent)>;
  using IdleListenerContainerType = std::list<IdleListenerType>;

  // This method may be called from multiple threads
  void finished(WorkerIDType id, WorkUnitCountType num = 1);

  // These methods should only be called by a single thread (i.e., comm thread)
  void enqueued(WorkUnitCountType num = 1);
  void registerIdleListener(IdleListenerType listener);
  void progress();

protected:
  void triggerListeners(eWorkerGroupEvent event);
  void updateConsumedTerm();

private:
  AtomicType<WorkUnitCountType> num_enqueued_ = {0};
  AtomicType<WorkUnitCountType> num_finished_ = {0};
  AtomicType<bool> maybe_idle_= {false};
  WorkUnitCountType num_consumed_ = 0;
  IdleListenerContainerType listeners_;
  eWorkerGroupEvent last_event_ = eWorkerGroupEvent::InvalidEvent;
};


}} /* end namespace vt::worker */

#endif /*INCLUDED_WORKER_WORKER_GROUP_COUNTER_H*/
