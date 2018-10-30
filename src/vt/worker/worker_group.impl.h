
#if !defined INCLUDED_WORKER_WORKER_GROUP_IMPL_H
#define INCLUDED_WORKER_WORKER_GROUP_IMPL_H

#include "config.h"
#include "context/context.h"
#include "worker_common.h"
#include "worker_group.h"

#include <functional>
#include <cstdint>

namespace vt { namespace worker {

template <typename WorkerT>
WorkerGroupAny<WorkerT>::WorkerGroupAny()
  : WorkerGroupAny(num_default_workers)
{ }

template <typename WorkerT>
WorkerGroupAny<WorkerT>::WorkerGroupAny(WorkerCountType const& in_num_workers)
  : num_workers_(in_num_workers)
{
  debug_print(
    worker, node,
    "WorkerGroup: constructing with default: num_workers_={}\n",
    num_workers_
  );

  initialize();
}

template <typename WorkerT>
void WorkerGroupAny<WorkerT>::initialize() {
  using namespace std::placeholders;
  finished_fn_ = std::bind(&WorkerGroupAny::finished, this, _1, _2);

  workers_.resize(num_workers_);
}

template <typename WorkerT>
bool WorkerGroupAny<WorkerT>::commScheduler() {
  return WorkerGroupComm::schedulerComm(finished_fn_);
}

template <typename WorkerT>
/*virtual*/ WorkerGroupAny<WorkerT>::~WorkerGroupAny() {
  if (workers_.size() > 0) {
    joinWorkers();
  }
}

template <typename WorkerT>
void WorkerGroupAny<WorkerT>::enqueueCommThread(WorkUnitType const& work_unit) {
  vtAssert(initialized_, "Must be initialized to enqueue");
  this->enqueued();
  WorkerGroupComm::enqueueComm(work_unit);
}

template <typename WorkerT>
void WorkerGroupAny<WorkerT>::enqueueAnyWorker(WorkUnitType const& work_unit) {
  vtAssert(initialized_, "Must be initialized to enqueue");

  this->enqueued();
  workers_[0].enqueue(work_unit);
}

template <typename WorkerT>
void WorkerGroupAny<WorkerT>::enqueueForWorker(
  WorkerIDType const& worker_id, WorkUnitType const& work_unit
) {
  vtAssert(initialized_, "Must be initialized to enqueue");
  vtAssert(worker_id < workers_.size(), "Worker ID must be valid");

  this->enqueued();
  workers_[worker_id]->enqueue(work_unit);
}

template <typename WorkerT>
void WorkerGroupAny<WorkerT>::enqueueAllWorkers(WorkUnitType const& work_unit) {
  vtAssert(initialized_, "Must be initialized to enqueue");

  this->enqueued(num_workers_);

  for (auto&& elm : workers_) {
    elm->enqueue(work_unit);
  }
}

template <typename WorkerT>
void WorkerGroupAny<WorkerT>::progress() {
  for (auto&& elm : workers_) {
    elm->progress();
  }

  WorkerGroupCounter::progress();
}

template <typename WorkerT>
void WorkerGroupAny<WorkerT>::spawnWorkersBlock(WorkerCommFnType comm_fn) {
  debug_print(
    worker, node,
    "WorkerGroup: spawnWorkersBlock: num_workers_={}\n", num_workers_
  );

  // spawn the workers
  spawnWorkers();

  // block the comm thread on the passed function
  comm_fn();
}

template <typename WorkerT>
void WorkerGroupAny<WorkerT>::spawnWorkers() {
  using namespace std::placeholders;

  debug_print(
    worker, node,
    "WorkerGroup: spawnWorkers: num_workers_={}\n", num_workers_
  );

  vtAssert(workers_.size() >= num_workers_, "Must be correct size");

  for (int i = 0; i < num_workers_; i++) {
    WorkerIDType const worker_id = i;
    workers_[i] = std::make_unique<WorkerT>(
      worker_id, num_workers_, finished_fn_
    );
  }

  for (auto&& elm : workers_) {
    elm->spawn();
  }

  initialized_ = true;

  // Give every worker at least one work unit (for termination purposes)
  auto initial_work_unit = []{};
  enqueueAllWorkers(initial_work_unit);
}

template <typename WorkerT>
void WorkerGroupAny<WorkerT>::joinWorkers() {
  debug_print(
    worker, node,
    "WorkerGroup: joinWorkers: num_workers_={}\n", num_workers_
  );

  for (auto&& elm : workers_) {
    elm->join();
  }

  workers_.clear();

  initialized_ = false;
}

}} /* end namespace vt::worker */

#endif /*INCLUDED_WORKER_WORKER_GROUP_IMPL_H*/
