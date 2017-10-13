
#include "config.h"
#include "context/context.h"
#include "worker/worker_common.h"
#include "worker/worker_openmp.h"

#include <thread>
#include <memory>
#include <functional>

namespace vt { namespace worker {

OMPWorker::OMPWorker(
  WorkerIDType const& in_worker_id_, WorkerIDType const& in_num_thds
) : worker_id_(in_worker_id_), num_thds_(in_num_thds)
{ }

void OMPWorker::enqueue(WorkUnitType const& work_unit) {
  work_queue_.pushBack(work_unit);
}

void OMPWorker::scheduler() {
  bool should_term_local = false;
  do {
    if (work_queue_.size() > 0) {
      auto elm = work_queue_.popGetBack();
      elm();
    }

    #pragma omp atomic read
    should_term_local = should_terminate_;
  } while (not should_term_local);
}

void OMPWorker::sendTerminateSignal() {
  #pragma omp atomic write
  should_terminate_ = true;
}

void OMPWorker::spawn() {
  debug_print(
    worker, node,
    "OMPWorker: spawn: spawning worker: id=%d\n", worker_id_
  );

  scheduler();
}

void OMPWorker::join() {
  debug_print(
    worker, node,
    "OMPWorker: join: spawning worker: id=%d\n", worker_id_
  );

  // tell the worker to return from the scheduler loop
  sendTerminateSignal();
}

void OMPWorker::dispatch(WorkerFunType fun) {
  enqueue(fun);
}

}} /* end namespace vt::worker */
