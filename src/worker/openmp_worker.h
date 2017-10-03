
#if !defined __RUNTIME_TRANSPORT_OPENMP_WORKER__
#define __RUNTIME_TRANSPORT_OPENMP_WORKER__

#include "config.h"
#include "worker/worker_common.h"
#include "worker/worker_types.h"
#include "utils/container/concurrent_deque.h"

#include <thread>
#include <functional>
#include <memory>
#include <atomic>

namespace vt { namespace worker {

struct OMPWorker {
  using WorkerFunType = std::function<void()>;
  using ThreadType = std::thread;
  using ThreadPtrType = std::unique_ptr<ThreadType>;
  using WorkUnitContainerType = util::container::ConcurrentDeque<WorkUnitType>;

  OMPWorker(WorkerIDType const& in_worker_id_, WorkerIDType const& in_num_thds);
  OMPWorker(OMPWorker const&) = delete;

  void spawn();
  void join();
  void dispatch(WorkerFunType fun);
  void enqueue(WorkUnitType const& work_unit);
  void sendTerminateSignal();

private:
  void scheduler();

private:
  bool should_terminate_= false;
  WorkerIDType worker_id_ = no_worker_id;
  WorkerIDType num_thds_ = no_worker_id;
  WorkUnitContainerType work_queue_;
  ThreadPtrType thd_ = nullptr;
};

}} /* end namespace vt::worker */

#if backend_check_enabled(detector)
  #include "worker/worker_traits.h"

  namespace vt { namespace worker {

  static_assert(
    WorkerTraits<OMPWorker>::is_worker,
    "vt::worker::Worker must follow the Worker concept"
  );

  }} /* end namespace vt::worker */
#endif

#endif /*__RUNTIME_TRANSPORT_OPENMP_WORKER__*/
