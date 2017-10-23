
#include "config.h"
#include "context/context.h"
#include "context/context_attorney.h"
#include "worker/worker_common.h"

namespace vt { namespace ctx {

/*static*/ void ContextAttorney::setWorker(WorkerIDType const worker) {
  theContext()->setWorker(worker);
}

/*static*/ void ContextAttorney::setNumWorkers(WorkerCountType const nworkers) {
  theContext()->setNumWorkers(nworkers);
}

}} /* end namespace vt::ctx */
