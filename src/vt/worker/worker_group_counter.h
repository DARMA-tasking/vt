
#if !defined INCLUDED_WORKER_WORKER_GROUP_COUNTER_H
#define INCLUDED_WORKER_WORKER_GROUP_COUNTER_H

#include "vt/config.h"
#include "vt/worker/worker_common.h"
#include "vt/utils/atomic/atomic.h"
#include "vt/utils/container/process_ready_buffer.h"

#include <list>

namespace vt { namespace worker {

using ::vt::util::atomic::AtomicType;
using ::vt::util::container::ProcessBuffer;

struct WorkerGroupCounter {
  using IdleListenerType = std::function<void(eWorkerGroupEvent)>;
  using IdleListenerContainerType = std::list<IdleListenerType>;
  using EnqueueCountContainerType = ProcessBuffer<WorkUnitCountType>;

  WorkerGroupCounter() {
    attachEnqueueProgressFn();
  }

  // This method may be called from multiple threads
  void enqueued(WorkUnitCountType num = 1);
  void finished(WorkerIDType id, WorkUnitCountType num = 1);

  // These methods should only be called by a single thread (i.e., comm thread)
  void enqueuedComm(WorkUnitCountType num = 1);
  void registerIdleListener(IdleListenerType listener);
  void progress();

protected:
  void assertCommThread();
  void attachEnqueueProgressFn();
  void triggerListeners(eWorkerGroupEvent event);
  void updateConsumedTerm();

private:
  AtomicType<WorkUnitCountType> num_enqueued_ = {0};
  AtomicType<WorkUnitCountType> num_finished_ = {0};
  AtomicType<bool> maybe_idle_= {false};
  WorkUnitCountType num_consumed_ = 0;
  IdleListenerContainerType listeners_;
  eWorkerGroupEvent last_event_ = eWorkerGroupEvent::InvalidEvent;
  EnqueueCountContainerType enqueued_count_;
};


}} /* end namespace vt::worker */

#endif /*INCLUDED_WORKER_WORKER_GROUP_COUNTER_H*/
