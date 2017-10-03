
#if !defined __RUNTIME_TRANSPORT_WORKER_GROUP_OMP__
#define __RUNTIME_TRANSPORT_WORKER_GROUP_OMP__

#include "config.h"

#if backend_check_enabled(openmp)

#include "worker/worker_common.h"
#include "worker/worker_types.h"
#include "worker/openmp_worker.h"

#include <vector>
#include <memory>
#include <functional>

namespace vt { namespace worker {

struct WorkerGroupOMP {
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
  void joinWorkers();

  void enqueueAnyWorker(WorkUnitType const& work_unit);
  void enqueueForWorker(
    WorkerIDType const& worker_id, WorkUnitType const& work_unit
  );
  void enqueueAllWorkers(WorkUnitType const& work_unit);

private:
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
#endif

#endif

#endif /*__RUNTIME_TRANSPORT_WORKER_GROUP_OMP__*/
