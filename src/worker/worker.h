
#if !defined INCLUDED_WORKER_WORKER_H
#define INCLUDED_WORKER_WORKER_H

#include "config.h"
#include "worker/worker_common.h"
#include "worker/worker_types.h"

#include <functional>

namespace vt { namespace worker {

struct Worker {
  using WorkerFunType = std::function<void()>;

  Worker(WorkerIDType const& in_worker_id_, WorkerCountType const& in_num_thds);
  Worker(Worker const&) = delete;

  void spawn();
  void join();
  void dispatch(WorkerFunType fun);
  void enqueue(WorkUnitType const& work_unit);
  void progress();
};

}} /* end namespace vt::worker */

#if backend_check_enabled(detector)
  #include "worker/worker_traits.h"

  namespace vt { namespace worker {

  static_assert(
    WorkerTraits<Worker>::is_worker,
    "vt::worker::Worker must follow the Worker concept"
  );

  }} /* end namespace vt::worker */
#endif /*backend_check_enabled(detector)*/

#endif /*INCLUDED_WORKER_WORKER_H*/
