
#include "config.h"

#if backend_check_enabled(openmp)

#include "worker/worker_common.h"
#include "worker/worker_group_omp.h"

#include <omp.h>

namespace vt { namespace worker {

WorkerGroupOMP::WorkerGroupOMP()
  : WorkerGroupOMP(num_default_workers)
{ }

WorkerGroupOMP::WorkerGroupOMP(WorkerCountType const& in_num_workers)
  : num_workers_(in_num_workers)
{
  initialize();
}

void WorkerGroupOMP::initialize() {
  worker_state_.resize(num_workers_);
}

/*virtual*/ WorkerGroupOMP::~WorkerGroupOMP() {
  worker_state_.clear();
}

void WorkerGroupOMP::spawnWorkers() {
  debug_print(
    worker, node,
    "Worker group OMP: launching num worker threads=%d, num comm threads=%d\n",
    num_workers_, num_default_comm
  );

  initialized_ = true;

  // @todo: should the total threads be used here?
  // auto const total_threads = num_workers_ + num_default_comm;

  #pragma omp parallel num_threads(num_workers_)
  {
    WorkerIDType const thd = omp_get_thread_num();
    WorkerIDType const nthds = omp_get_num_threads();

    debug_print(
      worker, node,
      "Worker group OMP: thd=%d, num threads=%d\n", thd, nthds
    );

    // if (thd == total_threads - 1) {
    // } else {
    worker_state_[thd] = std::make_unique<WorkerStateType>(thd, nthds);
    worker_state_[thd]->spawn();
    //}
  }
}

void WorkerGroupOMP::joinWorkers() {
  for (int i = 0; i < num_workers_; i++) {
    worker_state_[i]->sendTerminateSignal();
  }
}

void WorkerGroupOMP::enqueueAnyWorker(WorkUnitType const& work_unit) {
  assert(initialized_ and "Must be initialized to enqueue");

  worker_state_[0]->enqueue(work_unit);
}

void WorkerGroupOMP::enqueueForWorker(
  WorkerIDType const& worker_id, WorkUnitType const& work_unit
) {
  assert(initialized_ and "Must be initialized to enqueue");
  assert(worker_id < worker_state_.size() and "Worker ID must be valid");

  worker_state_[worker_id]->enqueue(work_unit);
}

void WorkerGroupOMP::enqueueAllWorkers(WorkUnitType const& work_unit) {
  assert(initialized_ and "Must be initialized to enqueue");

  for (auto&& elm : worker_state_) {
    elm->enqueue(work_unit);
  }
}

}} /* end namespace vt::worker */

#endif
