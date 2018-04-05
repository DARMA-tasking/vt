
#include "config.h"

#if backend_no_threading

#include "context/context.h"
#include "context/context_attorney.h"
#include "collective/collective_ops.h"
#include "worker/worker_common.h"
#include "worker/worker_seq.h"

#include <memory>
#include <functional>

#define WORKER_SEQ_VERBOSE 0

namespace vt { namespace worker {

using namespace fcontext;

WorkerSeq::WorkerSeq(
  WorkerIDType const& in_worker_id_, WorkerCountType const& in_num_thds,
  WorkerFinishedFnType finished_fn
) : worker_id_(in_worker_id_), num_thds_(in_num_thds), finished_fn_(finished_fn),
    stack(create_fcontext_stack())
{ }

void WorkerSeq::enqueue(WorkUnitType const& work_unit) {
  work_queue_.pushBack(work_unit);
}

void WorkerSeq::progress() {
  #if WORKER_SEQ_VERBOSE
  debug_print(worker, node, "WorkerSeq: progress: id=%d\n", worker_id_);
  #endif

  auto new_live = jump_fcontext(live.ctx);
  live = new_live;
}

/*static*/ void WorkerSeq::workerSeqSched(fcontext_transfer_t t) {
  using ::vt::ctx::ContextAttorney;

  void* data = t.data;
  WorkerSeq* seq = reinterpret_cast<WorkerSeq*>(data);
  fcontext_transfer_t cur = t;

  #if WORKER_SEQ_VERBOSE
  debug_print(worker, node, "workerSeqSched: id=%d\n", seq->worker_id_);
  #endif

  while (!seq->should_terminate_) {
    ContextAttorney::setWorker(seq->worker_id_);

    if (seq->work_queue_.size() > 0) {
      auto elm = seq->work_queue_.popGetBack();
      elm();
      seq->finished_fn_(seq->worker_id_, 1);
    }

    #if WORKER_SEQ_VERBOSE
    debug_print(worker, node, "seq sched: id=%d jump out\n", seq->worker_id_);
    #endif

    fcontext_transfer_t new_ctx = jump_fcontext(cur.ctx, nullptr);
    cur = new_ctx;

    #if WORKER_SEQ_VERBOSE
    debug_print(worker, node, "seq sched: id=%d jump in\n", seq->worker_id_);
    #endif
  }

  debug_print(worker, node, "seq sched: id=%d term\n", seq->worker_id_);
  jump_fcontext(cur.ctx, nullptr);
}

void WorkerSeq::startScheduler() {
  debug_print(worker, node, "WorkerSeq: startScheduler: id=%d\n", worker_id_);

  using ::vt::ctx::ContextAttorney;

  // Set the thread-local worker in the Context
  ContextAttorney::setWorker(worker_id_);

  fctx = make_fcontext_stack(stack, workerSeqSched);

  #if WORKER_SEQ_VERBOSE
  debug_print(worker, node, "WorkerSeq: jump context: id=%d\n", worker_id_);
  #endif

  live = jump_fcontext(fctx, this);

  #if WORKER_SEQ_VERBOSE
  debug_print(worker, node, "WorkerSeq: jump context out: id=%d\n", worker_id_);
  #endif
}

void WorkerSeq::sendTerminateSignal() {
  should_terminate_ = true;
}

void WorkerSeq::spawn() {
  debug_print(worker, node, "WorkerSeq: spawn: id=%d\n", worker_id_);
  startScheduler();
}

void WorkerSeq::join() {
  debug_print(worker, node, "WorkerSeq: join: id=%d\n", worker_id_);
  // tell the worker to return from the scheduler loop
  sendTerminateSignal();
  progress();
}

void WorkerSeq::dispatch(WorkerFunType fun) {
  enqueue(fun);
}

}} /* end namespace vt::worker */

#endif /*backend_no_threading*/

