
#if !defined INCLUDED_WORKER_WORKER_HEADERS_H
#define INCLUDED_WORKER_WORKER_HEADERS_H

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

extern worker::WorkerGroup* theWorkerGrp();

} /* end namespace vt */

#endif /*INCLUDED_WORKER_WORKER_HEADERS_H*/
