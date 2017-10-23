
#if !defined INCLUDED_CONTEXT_CONTEXT_ATTORNEY_H
#define INCLUDED_CONTEXT_CONTEXT_ATTORNEY_H

#include "config.h"
#include "context/context_attorney_fwd.h"
#include "worker/worker_headers.h"
#include "runtime/runtime_headers.h"

namespace vt {  namespace ctx {

struct ContextAttorney {
  // Allow the worker or worker group to modify the contextual worker
  friend worker::WorkerGroupType;
  friend worker::WorkerType;
  // Allow the runtime to set the number of workers
  friend runtime::Runtime;

private:
  static void setWorker(WorkerIDType const worker);
  static void setNumWorkers(WorkerCountType const worker_count);
};

}} /* end namespace vt::ctx */

#endif /*INCLUDED_CONTEXT_CONTEXT_ATTORNEY_H*/
