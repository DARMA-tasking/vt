
#include "config.h"
#include "context/context.h"
#include "worker/worker_common.h"
#include "worker/worker_group_counter.h"
#include "termination/term_headers.h"

#include <cassert>
#include <functional>

#define WORKER_COUNTER_VERBOSE 0

namespace vt { namespace worker {

void WorkerGroupCounter::attachEnqueueProgressFn() {
  using std::placeholders::_1;
  auto fn = std::bind(&WorkerGroupCounter::enqueued, this, _1);
  enqueued_count_.attach(fn, worker_id_comm_thread);
}

void WorkerGroupCounter::enqueued(WorkUnitCountType num) {
  auto const is_comm = theContext()->getWorker() == worker_id_comm_thread;
  if (is_comm) {
    enqueuedComm(num);
  } else {
    enqueued_count_.push(num);
  }
}

void WorkerGroupCounter::enqueuedComm(WorkUnitCountType num) {
  assertCommThread();

  theTerm()->produce(term::any_epoch_sentinel, num);

  num_enqueued_ += num;
  maybe_idle_.store(false);

  // Call progress to update in the case where state was previously reported as
  // idle to the listeners
  progress();
}

void WorkerGroupCounter::finished(WorkerIDType id, WorkUnitCountType num) {
  #if WORKER_COUNTER_VERBOSE
  debug_print(
    worker, node,
    "WorkerGroupCounter: completed: id=%d, num=%lld\n", id, num
  );
  #endif

  // This method may be called from multiple threads
  auto const cur_finished = num_finished_.fetch_add(num) + num;
  auto const cur_enqueued = num_enqueued_.load();
  bool const is_idle = cur_finished == cur_enqueued;
  if (is_idle) {
    maybe_idle_.store(is_idle);
  }
}

void WorkerGroupCounter::assertCommThread() {
  // Sanity check to ensure single-threaded methods are only called by the
  // communication thread
  assert(
    theContext()->getWorker() == worker_id_comm_thread &&
    "This must only run on the communication thread"
  );
}

void WorkerGroupCounter::registerIdleListener(IdleListenerType listener) {
  assertCommThread();
  listeners_.push_back(listener);
}

void WorkerGroupCounter::progress() {
  bool const cur_maybe_idle = maybe_idle_.load();
  bool const last_event_idle = last_event_ == eWorkerGroupEvent::WorkersIdle;

  assertCommThread();

  enqueued_count_.progress();

  if (cur_maybe_idle || last_event_idle) {
    auto const cur_finished = num_finished_.load();
    auto const cur_enqueued = num_enqueued_.load();
    bool const is_idle = cur_finished == cur_enqueued;

    #if WORKER_COUNTER_VERBOSE
    debug_print(
      worker, node,
      "WorkerGroupCounter: progress: fin=%lld, enq=%lld, is_idle=%s, "
      "last_event=%s, last_event_idle=%s\n",
      cur_finished, cur_enqueued, print_bool(is_idle),
      WORKER_GROUP_EVENT_STR(last_event_), print_bool(last_event_idle)
    );
    #endif

    if (is_idle && !last_event_idle) {
      // trigger listeners
      triggerListeners(eWorkerGroupEvent::WorkersIdle);
    } else if (!is_idle) {
      if (last_event_idle) {
        triggerListeners(eWorkerGroupEvent::WorkersBusy);
      }
      maybe_idle_.store(is_idle);
    } else if (is_idle && num_finished_.load() > num_consumed_) {
      updateConsumedTerm();
    }
  }
}

void WorkerGroupCounter::updateConsumedTerm() {
  auto const cur_finished = num_finished_.load();
  auto const cur_consumed = num_consumed_;
  auto const num_to_consume = cur_finished - cur_consumed;

  num_consumed_ += num_to_consume;

  debug_print(
    worker, node,
    "WorkerGroupCounter: updating: num_fin=%lld, num_con=%lld, num_sub=%lld\n",
    cur_finished, cur_consumed, num_to_consume
  );

  theTerm()->consume(term::any_epoch_sentinel, num_to_consume);
}

void WorkerGroupCounter::triggerListeners(eWorkerGroupEvent event) {
  #if WORKER_COUNTER_VERBOSE
  debug_print(
    worker, node,
    "WorkerGroupCounter: triggering listeners: event=%s\n",
    WORKER_GROUP_EVENT_STR(event)
  );
  #endif

  if (event == eWorkerGroupEvent::WorkersIdle) {
    updateConsumedTerm();
  }

  last_event_ = event;

  for (auto&& elm : listeners_) {
    elm(event);
  }
}

}} /* end namespace vt::worker */
