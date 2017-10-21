
#if !defined INCLUDED_WORKER_WORKER_GROUP_FWD_H
#define INCLUDED_WORKER_WORKER_GROUP_FWD_H

#include "config.h"

namespace vt { namespace worker {

#if backend_check_enabled(openmp)
  struct WorkerGroupOMP;
#else
  struct WorkerGroupSTD;
#endif

#if backend_check_enabled(openmp)
  using WorkerGroup = WorkerGroupOMP;
#else
  using WorkerGroup = WorkerGroupSTD;
#endif

}} /* end namespace vt::worker */

#endif /*INCLUDED_WORKER_WORKER_GROUP_FWD_H*/
