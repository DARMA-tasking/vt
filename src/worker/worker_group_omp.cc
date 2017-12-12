
#include "config.h"
#include "context/context.h"
#include "context/context_attorney.h"
#include "collective/collective.h"

#include "Kokkos_Core.hpp"
#include "Kokkos_OpenMP.hpp"

#if backend_check_enabled(openmp)

#include "worker/worker_common.h"
#include "worker/worker_group_omp.h"

#include <functional>

#include <omp.h>

#define WORKER_OMP_VERBOSE 1

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
  using namespace std::placeholders;
  finished_fn_ = std::bind(&WorkerGroupOMP::finished, this, _1, _2);

  worker_state_.resize(num_workers_);
}

bool WorkerGroupOMP::commScheduler() {
  return WorkerGroupComm::schedulerComm(finished_fn_);
}

void WorkerGroupOMP::progress() {
  WorkerGroupCounter::progress();
}

/*virtual*/ WorkerGroupOMP::~WorkerGroupOMP() {
  worker_state_.clear();
}

void WorkerGroupOMP::enqueueCommThread(WorkUnitType const& work_unit) {
  this->enqueued();
  WorkerGroupComm::enqueueComm(work_unit);
}

void WorkerGroupOMP::doWork(
  int const parent, int const pnthds, WorkerCommFnType comm_fn,
  bool const hasCommThread
) {
  using ::vt::ctx::ContextAttorney;

  WorkerIDType const thd = omp_get_thread_num() + (parent * pnthds);
  WorkerIDType const nthds = omp_get_num_threads();

  // For now, all workers to have direct access to the runtime
  // TODO: this needs to change
  CollectiveOps::setCurrentRuntimeTLS();

  if (thd < num_workers_ or not hasCommThread) {
    // Set the thread-local worker in Context
    ContextAttorney::setWorker(thd);

    debug_print(
      worker, node,
      "Worker group OMP: (worker) thd=%d, worker=%d, num threads=%d, hasComm=%s\n",
      thd, theContext()->getWorker(), num_workers_, print_bool(hasCommThread)
    );

    worker_state_[thd] = std::make_unique<WorkerStateType>(
      thd, nthds, finished_fn_
    );
    ready_++;
    worker_state_[thd]->spawn();
  } else {
    // Set the thread-local worker in Context
    ContextAttorney::setWorker(worker_id_comm_thread);

    debug_print(
      worker, node,
      "Worker group OMP: (comm) thd=%d, num threads=%d\n", thd, nthds
    );

    // Wait until all the workers are created and have filled the
    // worker_state_ vector
    while (ready_.load() < num_workers_) ;

    debug_print(
      worker, node,
      "Worker group OMP: (comm) thd=%d, num threads=%d creating initial\n",
      thd, nthds
    );

    // Enqueue an initial work unit for termination purposes
    auto initial_work_unit = []{};
    enqueueAllWorkers(initial_work_unit);

    // launch comm function on the main communication thread
    comm_fn();

    debug_print(worker, node, "comm: should call join\n");

    // once the comm function exits the program is terminated
    for (auto thd = 0; thd < num_workers_; thd++) {
      debug_print(worker, node, "comm: calling join thd=%d\n", thd );
      worker_state_[thd]->join();
    }
  }
}

void WorkerGroupOMP::spawnWorkersBlock(WorkerCommFnType comm_fn) {
  using ::vt::ctx::ContextAttorney;

  debug_print(
    worker, node,
    "Worker group OMP: launching num worker threads=%d, num comm threads=%d\n",
    num_workers_, num_default_comm
  );

  initialized_ = true;

  debug_print(
    worker, node,
    "worker group OMP spawning=%d\n", num_workers_ + 1
  );

  // set as constant for now
  int const partition_size = 2;
  int const num_partitions = (num_workers_ + 1)/partition_size;

  printf("num_partitions=%d\n",num_partitions);

  Kokkos::initialize();

  Kokkos::OpenMP::partition_master([this,comm_fn,num_partitions](int id, int num){
    printf("id=%d,num=%d\n",id,num);
    // Communication thread partition
    #pragma omp parallel num_threads(num)
    {
      auto const comm_thread = id == num_partitions-1;
      doWork(id, num, comm_fn, comm_thread);
    }
  }, num_partitions, partition_size);
}

void WorkerGroupOMP::spawnWorkers() {
  assert(0 and "Not supported on OMP workers");
}

void WorkerGroupOMP::joinWorkers() {
  for (int i = 0; i < num_workers_; i++) {
    worker_state_[i]->sendTerminateSignal();
  }
}

void WorkerGroupOMP::enqueueAnyWorker(WorkUnitType const& work_unit) {
  assert(initialized_ and "Must be initialized to enqueue");

  #if WORKER_OMP_VERBOSE
  debug_print(worker, node, "WorkerGroupOMP: enqueue any worker\n");
  #endif

  this->enqueued();
  worker_state_[0]->enqueue(work_unit);
}

void WorkerGroupOMP::enqueueForWorker(
  WorkerIDType const& worker_id, WorkUnitType const& work_unit
) {
  assert(initialized_ and "Must be initialized to enqueue");
  assert(worker_id < worker_state_.size() and "Worker ID must be valid");

  #if WORKER_OMP_VERBOSE
  debug_print(worker, node, "WorkerGroupOMP: enqueue for id=%d\n", worker_id);
  #endif

  this->enqueued();
  worker_state_[worker_id]->enqueue(work_unit);
}

void WorkerGroupOMP::enqueueAllWorkers(WorkUnitType const& work_unit) {
  assert(initialized_ and "Must be initialized to enqueue");

  #if WORKER_OMP_VERBOSE
  debug_print(worker, node, "WorkerGroupOMP: enqueue all workers\n");
  #endif

  this->enqueued(num_workers_);

  printf("enqueue for worker: num_workers_=%d\n",num_workers_);

  for (auto&& elm : worker_state_) {
    printf("enqueue for worker\n");
    fflush(stdout);
    elm->enqueue(work_unit);
  }
}

}} /* end namespace vt::worker */

#endif
