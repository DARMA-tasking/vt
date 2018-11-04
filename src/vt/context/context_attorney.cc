
#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/context/context_attorney.h"
#include "vt/worker/worker_common.h"

namespace vt { namespace ctx {

/*static*/ void ContextAttorney::setWorker(WorkerIDType const worker) {
  theContext()->setWorker(worker);
}

/*static*/ void ContextAttorney::setNumWorkers(WorkerCountType const nworkers) {
  theContext()->setNumWorkers(nworkers);
}

}} /* end namespace vt::ctx */
