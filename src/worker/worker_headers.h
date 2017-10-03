
#if !defined __RUNTIME_TRANSPORT_WORKER_HEADERS__
#define __RUNTIME_TRANSPORT_WORKER_HEADERS__

#include "worker/worker.h"
#include "worker/worker_group.h"
#include "worker/worker_group_omp.h"
#include "worker/worker_types.h"

namespace vt { namespace worker {

#if backend_check_enabled(openmp)
  using WorkerGroup = WorkerGroupOMP;
#else
  using WorkerGroup = WorkerGroupStd;
#endif

}} /* end namespace vt::worker */

namespace vt {

extern std::unique_ptr<worker::WorkerGroup> theWorkerGrp;

} /* end namespace vt */

#endif /*__RUNTIME_TRANSPORT_WORKER_HEADERS__*/
