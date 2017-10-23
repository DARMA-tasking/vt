
#include "config.h"
#include "context/context.h"
#include "worker/worker_common.h"
#include "worker/worker_group_counter.h"

namespace vt { namespace worker {

void WorkerGroupCounter::enqueued(WorkUnitCountType num) {
  num_enqueued_ += num;
  maybe_idle_.store(false);

  // Call progress to update in the case where state was previously reported as
  // idle to the listeners
  progress();
}

void WorkerGroupCounter::finished(WorkerIDType id, WorkUnitCountType num) {
  debug_print(
    worker, node,
    "WorkerGroupCounter: finished: id=%d, num=%lld\n", id, num
  );

  // This method may be called from multiple threads
  auto const cur_finished = num_finished_.fetch_add(num) + num;
  auto const cur_enqueued = num_enqueued_.load();
  bool const is_idle = cur_finished == cur_enqueued;
  if (is_idle) {
    maybe_idle_.store(is_idle);
  }
}

void WorkerGroupCounter::registerIdleListener(IdleListenerType listener) {
  listeners_.push_back(listener);
}

void WorkerGroupCounter::progress() {
  bool const cur_maybe_idle = maybe_idle_.load();
  bool const last_event_idle = last_event_ == eWorkerGroupEvent::WorkersIdle;

  if (cur_maybe_idle || last_event_idle) {
    auto const cur_finished = num_finished_.load();
    auto const cur_enqueued = num_enqueued_.load();
    bool const is_idle = cur_finished == cur_enqueued;

    debug_print(
      worker, node,
      "WorkerGroupCounter: progress: fin=%lld, enq=%lld, is_idle=%s, "
      "last_event=%s, last_event_idle=%s\n",
      cur_finished, cur_enqueued, print_bool(is_idle),
      WORKER_GROUP_EVENT_STR(last_event_), print_bool(last_event_idle)
    );

    if (is_idle && !last_event_idle) {
      // trigger listeners
      triggerListeners(eWorkerGroupEvent::WorkersIdle);
    } else if (!is_idle) {
      if (last_event_idle) {
        triggerListeners(eWorkerGroupEvent::WorkersBusy);
      }
      maybe_idle_.store(is_idle);
    }
  }
}

void WorkerGroupCounter::triggerListeners(eWorkerGroupEvent event) {
  debug_print(
    worker, node,
    "WorkerGroupCounter: triggering listeners: event=%s\n",
    WORKER_GROUP_EVENT_STR(event)
  );

  last_event_ = event;

  for (auto&& elm : listeners_) {
    elm(event);
  }
}

}} /* end namespace vt::worker */
