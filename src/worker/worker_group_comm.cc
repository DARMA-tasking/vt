
#include "config.h"
#include "context/context.h"
#include "worker/worker_common.h"
#include "worker/worker_group_comm.h"

namespace vt { namespace worker {

void WorkerGroupComm::enqueueCommThread(WorkUnitType const& work_unit) {
  comm_work_deque_.pushBack(work_unit);
}

bool WorkerGroupComm::commScheduler() {
  bool found = false;
  if (comm_work_deque_.size() > 0) {
    debug_print(
      worker, node,
      "WorkerGroupComm: scheduler executing size=%ld\n", comm_work_deque_.size()
    );

    auto work_unit = comm_work_deque_.front();
    comm_work_deque_.popFront();
    work_unit();
    found = true;
  }
  return found;
}

}} /* end namespace vt::worker */
