
#if !defined INCLUDED_WORKER_WORKER_SEQ_H
#define INCLUDED_WORKER_WORKER_SEQ_H

#include "vt/config.h"

#if backend_no_threading

#include "vt/worker/worker_common.h"
#include "vt/worker/worker_types.h"
#include "vt/utils/container/concurrent_deque.h"

#include <fcontext.h>

#include <functional>

namespace vt { namespace worker {

struct WorkerSeq {
  using WorkerFunType = std::function<void()>;
  using WorkUnitContainerType = util::container::ConcurrentDeque<WorkUnitType>;

  WorkerSeq(
    WorkerIDType const& in_worker_id_, WorkerCountType const& in_num_thds,
    WorkerFinishedFnType finished_fn
  );
  WorkerSeq(WorkerSeq const&) = delete;

  void spawn();
  void join();
  void dispatch(WorkerFunType fun);
  void enqueue(WorkUnitType const& work_unit);
  void sendTerminateSignal();
  void progress();

private:
  void startScheduler();
  void continueScheduler();

  static void workerSeqSched(fcontext_transfer_t t);

private:
  bool should_terminate_ = false;
  WorkerIDType worker_id_ = no_worker_id;
  WorkerCountType num_thds_ = no_workers;
  WorkUnitContainerType work_queue_;
  WorkerFinishedFnType finished_fn_ = nullptr;
  fcontext_stack_t stack;
  fcontext_t fctx;
  fcontext_transfer_t live;
};

}} /* end namespace vt::worker */

#if backend_check_enabled(detector)
  #include "vt/worker/worker_traits.h"

  namespace vt { namespace worker {

  static_assert(
    WorkerTraits<WorkerSeq>::is_worker,
    "vt::worker::Worker must follow the Worker concept"
  );

  }} /* end namespace vt::worker */
#endif /*backend_check_enabled(detector)*/

#endif /*backend_no_threading*/

#endif /*INCLUDED_WORKER_WORKER_SEQ_H*/
