
#if !defined INCLUDED_WORKER_WORKER_STDTHREAD_H
#define INCLUDED_WORKER_WORKER_STDTHREAD_H

#include "vt/config.h"

#if backend_check_enabled(stdthread)

#include "vt/worker/worker_common.h"
#include "vt/worker/worker_types.h"
#include "vt/utils/container/concurrent_deque.h"

#include <thread>
#include <functional>
#include <memory>
#include <atomic>

namespace vt { namespace worker {

struct StdThreadWorker {
  using WorkerFunType = std::function<void()>;
  using ThreadType = std::thread;
  using ThreadPtrType = std::unique_ptr<ThreadType>;
  using WorkUnitContainerType = util::container::ConcurrentDeque<WorkUnitType>;

  StdThreadWorker(
    WorkerIDType const& in_worker_id_, WorkerCountType const& in_num_thds,
    WorkerFinishedFnType finished_fn
  );
  StdThreadWorker(StdThreadWorker const&) = delete;

  void spawn();
  void join();
  void dispatch(WorkerFunType fun);
  void enqueue(WorkUnitType const& work_unit);
  void sendTerminateSignal();
  void progress();

private:
  void scheduler();

private:
  std::atomic<bool> should_terminate_ = {false};
  WorkerIDType worker_id_ = no_worker_id;
  WorkerCountType num_thds_ = no_workers;
  WorkUnitContainerType work_queue_;
  ThreadPtrType thd_ = nullptr;
  WorkerFinishedFnType finished_fn_ = nullptr;
};

}} /* end namespace vt::worker */

#if backend_check_enabled(detector)
  #include "vt/worker/worker_traits.h"

  namespace vt { namespace worker {

  static_assert(
    WorkerTraits<StdThreadWorker>::is_worker,
    "vt::worker::Worker must follow the Worker concept"
  );

  }} /* end namespace vt::worker */
#endif

#endif /*backend_check_enabled(stdthread)*/

#endif /*INCLUDED_WORKER_WORKER_STDTHREAD_H*/
