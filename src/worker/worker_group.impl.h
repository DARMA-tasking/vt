
#if !defined __RUNTIME_TRANSPORT_WORKER_GROUP_IMPL__
#define __RUNTIME_TRANSPORT_WORKER_GROUP_IMPL__

#include "config.h"
#include "context.h"
#include "worker_common.h"
#include "worker_group.h"

#include <thread>
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
    "WorkerGroup: constructing with default: num_workers_=%u\n",
    num_workers_
  );

  initialize();
}

template <typename WorkerT>
void WorkerGroupAny<WorkerT>::initialize() {
  workers_.resize(num_workers_);
}

template <typename WorkerT>
/*virtual*/ WorkerGroupAny<WorkerT>::~WorkerGroupAny() {
  if (workers_.size() > 0) {
    joinWorkers();
  }
}

template <typename WorkerT>
void WorkerGroupAny<WorkerT>::enqueueAnyWorker(WorkUnitType const& work_unit) {
  assert(initialized_ and "Must be initialized to enqueue");

  workers_[0].enqueue(work_unit);
}

template <typename WorkerT>
void WorkerGroupAny<WorkerT>::enqueueForWorker(
  WorkerIDType const& worker_id, WorkUnitType const& work_unit
) {
  assert(initialized_ and "Must be initialized to enqueue");
  assert(worker_id < workers_.size() and "Worker ID must be valid");

  workers_[worker_id].enqueue(work_unit);
}

template <typename WorkerT>
void WorkerGroupAny<WorkerT>::enqueueAllWorkers(WorkUnitType const& work_unit) {
  assert(initialized_ and "Must be initialized to enqueue");

  for (auto&& elm : workers_) {
    elm.enqueue(work_unit);
  }
}

template <typename WorkerT>
void WorkerGroupAny<WorkerT>::spawnWorkers() {
  debug_print(
    worker, node,
    "WorkerGroup: spawnWorkers: num_workers_=%u\n", num_workers_
  );

  assert(workers_.size() >= num_workers_ and "Must be correct size");

  for (int i = 0; i < num_workers_; i++) {
    WorkerIDType const worker_id = i;
    workers_[i] = std::make_unique<WorkerT>(worker_id);
  }

  for (auto&& elm : workers_) {
    elm->spawn();
  }

  initialized_ = true;
}

template <typename WorkerT>
void WorkerGroupAny<WorkerT>::joinWorkers() {
  debug_print(
    worker, node,
    "WorkerGroup: joinWorkers: num_workers_=%u\n", num_workers_
  );

  for (auto&& elm : workers_) {
    elm->join();
  }

  workers_.clear();

  initialized_ = false;
}

}} /* end namespace vt::worker */

#endif /*__RUNTIME_TRANSPORT_WORKER_GROUP_IMPL__*/
