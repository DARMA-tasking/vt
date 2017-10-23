
#if !defined INCLUDED_WORKER_WORKER_HEADERS_H
#define INCLUDED_WORKER_WORKER_HEADERS_H

#include "worker/worker.h"
#include "worker/worker_group.h"
#include "worker/worker_group_omp.h"
#include "worker/worker_types.h"

namespace vt { namespace worker {

#if backend_check_enabled(openmp)
  using WorkerGroupType = WorkerGroupOMP;
#elif backend_check_enabled(stdthread)
  using WorkerGroupType = WorkerGroupSTD;
#endif

#if backend_check_enabled(openmp)
  using WorkerType = OMPWorker;
#elif backend_check_enabled(stdthread)
  using WorkerType = StdThreadWorker;
#endif

}} /* end namespace vt::worker */

namespace vt {

extern worker::WorkerGroupType* theWorkerGrp();

} /* end namespace vt */

#endif /*INCLUDED_WORKER_WORKER_HEADERS_H*/
