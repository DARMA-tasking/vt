
#if !defined INCLUDED_WORKER_WORKER_GROUP_H
#define INCLUDED_WORKER_WORKER_GROUP_H

#include "config.h"
#include "worker/worker_common.h"
#include "worker/worker.h"
#include "worker/worker_group_counter.h"
#include "worker/worker_group_comm.h"
#include "utils/atomic/atomic.h"

#if backend_check_enabled(stdthread)
  #include "worker/worker_stdthread.h"
#elif backend_no_threading
  #include "worker/worker_seq.h"
#endif

#include <vector>
#include <memory>

namespace vt { namespace worker {

using ::vt::util::atomic::AtomicType;

template <typename WorkerT>
struct WorkerGroupAny : WorkerGroupCounter, WorkerGroupComm {
  using WorkerType = WorkerT;
  using WorkerPtrType = std::unique_ptr<WorkerT>;
  using WorkerContainerType = std::vector<WorkerPtrType>;

  WorkerGroupAny();
  WorkerGroupAny(WorkerCountType const& in_num_workers);

  virtual ~WorkerGroupAny();

  void initialize();
  void spawnWorkers();
  void spawnWorkersBlock(WorkerCommFnType fn);
  void joinWorkers();
  void progress();

  bool commScheduler();
  void enqueueCommThread(WorkUnitType const& work_unit);
  void enqueueAnyWorker(WorkUnitType const& work_unit);
  void enqueueForWorker(
    WorkerIDType const& worker_id, WorkUnitType const& work_unit
  );
  void enqueueAllWorkers(WorkUnitType const& work_unit);

private:
  WorkerFinishedFnType finished_fn_ = nullptr;
  bool initialized_ = false;
  WorkerCountType num_workers_ = 0;
  WorkerContainerType workers_;
};

#if backend_check_enabled(stdthread)
  using WorkerGroupSTD = WorkerGroupAny<StdThreadWorker>;
#elif backend_no_threading
  using WorkerGroupSeq = WorkerGroupAny<WorkerSeq>;
#endif /*backend_check_enabled(stdthread)*/

}} /* end namespace vt::worker */

#if backend_check_enabled(detector) && backend_check_enabled(stdthread)
  #include "worker/worker_group_traits.h"

  namespace vt { namespace worker {

  static_assert(
    WorkerGroupTraits<WorkerGroupSTD>::is_worker,
    "WorkerGroupSTD must follow the WorkerGroup concept"
  );

  }} /* end namespace vt::worker */
#endif /*backend_check_enabled(detector) && backend_check_enabled(stdthread)*/

#include "worker/worker_group.impl.h"

#endif /*INCLUDED_WORKER_WORKER_GROUP_H*/
