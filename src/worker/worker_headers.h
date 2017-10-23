
#if !defined INCLUDED_WORKER_WORKER_HEADERS_H
#define INCLUDED_WORKER_WORKER_HEADERS_H

#include "worker/worker.h"
#include "worker/worker_group.h"
#include "worker/worker_types.h"

#if backend_check_enabled(openmp)
  #include "worker/worker_group_omp.h"
#elif backend_check_enabled(stdthread)
  #include "worker/worker_group.h"
#elif backend_no_threading
  #include "worker/worker_group.h"
#else
  backend_static_assert_unreachable
#endif

namespace vt { namespace worker {

#if backend_check_enabled(openmp)
  using WorkerGroupType = WorkerGroupOMP;
#elif backend_check_enabled(stdthread)
  using WorkerGroupType = WorkerGroupSTD;
#elif backend_no_threading
  using WorkerGroupType = WorkerGroupSeq;
#else
  backend_static_assert_unreachable
#endif

#if backend_check_enabled(openmp)
  using WorkerType = OMPWorker;
#elif backend_check_enabled(stdthread)
  using WorkerType = StdThreadWorker;
#elif backend_no_threading
  using WorkerType = WorkerSeq;
#else
  backend_static_assert_unreachable
#endif

}} /* end namespace vt::worker */

namespace vt {

extern worker::WorkerGroupType* theWorkerGrp();

} /* end namespace vt */

#endif /*INCLUDED_WORKER_WORKER_HEADERS_H*/
