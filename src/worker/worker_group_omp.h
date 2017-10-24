
#if !defined INCLUDED_WORKER_WORKER_GROUP_OMP_H
#define INCLUDED_WORKER_WORKER_GROUP_OMP_H

#include "config.h"

#if backend_check_enabled(openmp)

#include "worker/worker_common.h"
#include "worker/worker_types.h"
#include "worker/worker_openmp.h"
#include "worker/worker_group_counter.h"
#include "worker/worker_group_comm.h"

#include <vector>
#include <memory>
#include <functional>

namespace vt { namespace worker {

struct WorkerGroupOMP : WorkerGroupCounter, WorkerGroupComm {
  using WorkerType = OMPWorker;
  using WorkerStateType = WorkerType;
  using WorkerStatePtrType = std::unique_ptr<WorkerStateType>;
  using WorkerStateContainerType = std::vector<WorkerStatePtrType>;
  using WorkerFunType = std::function<void()>;
  using WorkUnitContainerType = util::container::ConcurrentDeque<WorkUnitType>;

  WorkerGroupOMP();
  WorkerGroupOMP(WorkerCountType const& in_num_workers);

  virtual ~WorkerGroupOMP();

  void initialize();
  void spawnWorkers();
  void spawnWorkersBlock(WorkerCommFnType fn);
  void joinWorkers();
  void progress();
  bool commScheduler();

  // // thread-safe comm thread enqueue
  // void enqueueCommThreadSafe(WorkUnitType const& work_unit);

  // non-thread-safe comm and worker thread enqueue
  void enqueueCommThread(WorkUnitType const& work_unit);
  void enqueueAnyWorker(WorkUnitType const& work_unit);
  void enqueueForWorker(
    WorkerIDType const& worker_id, WorkUnitType const& work_unit
  );
  void enqueueAllWorkers(WorkUnitType const& work_unit);

private:
  WorkerFinishedFnType finished_fn_ = nullptr;
  AtomicType<WorkerCountType> ready_ = {0};
  bool initialized_ = false;
  WorkerCountType num_workers_ = 0;
  WorkerStateContainerType worker_state_;
};

}} /* end namespace vt::worker */

#if backend_check_enabled(detector)
  #include "worker/worker_group_traits.h"

  namespace vt { namespace worker {

  static_assert(
    WorkerGroupTraits<WorkerGroupOMP>::is_worker,
    "WorkerGroupOMP must follow the WorkerGroup concept"
  );

  }} /* end namespace vt::worker */
#endif /*backend_check_enabled(detector)*/

#endif /*backend_check_enabled(openmp)*/

#endif /*INCLUDED_WORKER_WORKER_GROUP_OMP_H*/
