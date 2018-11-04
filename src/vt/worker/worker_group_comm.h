
#if !defined INCLUDED_WORKER_WORKER_GROUP_COMM_H
#define INCLUDED_WORKER_WORKER_GROUP_COMM_H

#include "vt/config.h"
#include "vt/worker/worker_common.h"
#include "vt/worker/worker_types.h"
#include "vt/utils/container/concurrent_deque.h"

namespace vt { namespace worker {

using ::vt::util::container::ConcurrentDeque;

struct WorkerGroupComm {
  void enqueueComm(WorkUnitType const& work_unit);
  bool schedulerComm(WorkerFinishedFnType finished_fn);

protected:
  ConcurrentDeque<WorkUnitType> comm_work_deque_;
};

}} /* end namespace vt::worker */

#endif /*INCLUDED_WORKER_WORKER_GROUP_COMM_H*/
