
#include "vt/config.h"

#if backend_check_enabled(openmp)

#include "vt/context/context.h"
#include "vt/worker/worker_common.h"
#include "vt/worker/worker_openmp.h"

#include <memory>
#include <functional>

#include <omp.h>

#define DEBUG_OMP_WORKER_SCHEDULER 0

namespace vt { namespace worker {

OMPWorker::OMPWorker(
  WorkerIDType const& in_worker_id_, WorkerIDType const& in_num_thds,
  WorkerFinishedFnType finished_fn
) : worker_id_(in_worker_id_), num_thds_(in_num_thds), finished_fn_(finished_fn)
{ }

void OMPWorker::enqueue(WorkUnitType const& work_unit) {
  work_queue_.pushBack(work_unit);
}

void OMPWorker::progress() {
  // Noop
}

void OMPWorker::scheduler() {
  bool should_term_local = false;
  do {
    if (work_queue_.size() > 0) {
      #if DEBUG_OMP_WORKER_SCHEDULER
      debug_print(
        worker, node,
        "OMPWorker: scheduler: size={}\n", work_queue_.size()
      );
      #endif

      auto elm = work_queue_.popGetBack();
      elm();
      finished_fn_(worker_id_, 1);
    }

    #pragma omp atomic read
    should_term_local = should_terminate_;
  } while (not should_term_local);
}

void OMPWorker::sendTerminateSignal() {
  #pragma omp atomic write
  should_terminate_ = true;

  debug_print(worker, node, "OMPWorker: sendTerminateSignal\n");
}

void OMPWorker::spawn() {
  debug_print(
    worker, node,
    "OMPWorker: spawn: spawning worker: id={}\n", worker_id_
  );

  scheduler();
}

void OMPWorker::join() {
  debug_print(
    worker, node,
    "OMPWorker: join: spawning worker: id={}\n", worker_id_
  );

  // tell the worker to return from the scheduler loop
  sendTerminateSignal();
}

void OMPWorker::dispatch(WorkerFunType fun) {
  enqueue(fun);
}

}} /* end namespace vt::worker */

#endif /*backend_check_enabled(openmp)*/
