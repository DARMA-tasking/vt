
#include "config.h"

#if backend_check_enabled(stdthread)

#include "context/context.h"
#include "context/context_attorney.h"
#include "collective/collective.h"
#include "worker/worker_common.h"
#include "worker/worker_stdthread.h"

#include <thread>
#include <memory>
#include <functional>

namespace vt { namespace worker {

StdThreadWorker::StdThreadWorker(
  WorkerIDType const& in_worker_id_, WorkerCountType const& in_num_thds,
  WorkerFinishedFnType finished_fn
) : worker_id_(in_worker_id_), num_thds_(in_num_thds), finished_fn_(finished_fn)
{ }

void StdThreadWorker::enqueue(WorkUnitType const& work_unit) {
  work_queue_.pushBack(work_unit);
}

void StdThreadWorker::scheduler() {
  using ::vt::ctx::ContextAttorney;

  // For now, all workers to have direct access to the runtime
  // TODO: this needs to change
  CollectiveOps::setCurrentRuntimeTLS();

  // Set the thread-local worker in the Context
  ContextAttorney::setWorker(worker_id_comm_thread);

  while (not should_terminate_.load()) {
    if (work_queue_.size() > 0) {
      auto elm = work_queue_.popGetBack();
      elm();
      finished_fn_(worker_id_, 1);
    }
  }
}

void StdThreadWorker::sendTerminateSignal() {
  should_terminate_.store(true);
}

void StdThreadWorker::spawn() {
  debug_print(
    worker, node,
    "StdThreadWorker: spawn: spawning worker: id=%d\n", worker_id_
  );

  auto sched_fn = std::bind(&StdThreadWorker::scheduler, this);
  thd_ = std::make_unique<ThreadType>(sched_fn);
}

void StdThreadWorker::join() {
  debug_print(
    worker, node,
    "StdThreadWorker: join: spawning worker: id=%d\n", worker_id_
  );

  // tell the worker to return from the scheduler loop
  sendTerminateSignal();

  // join the std::thread
  thd_->join();
}

void StdThreadWorker::dispatch(WorkerFunType fun) {
  enqueue(fun);
}

}} /* end namespace vt::worker */

#endif /*backend_check_enabled(stdthread)*/
