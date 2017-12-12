
#if !defined INCLUDED_WORKER_WORKER_COMMON_H
#define INCLUDED_WORKER_WORKER_COMMON_H

#include "config.h"

#include <cstdint>
#include <functional>

namespace vt { namespace worker {

static constexpr WorkerCountType const num_default_workers = 3;
static constexpr WorkerCountType const num_default_comm = 1;

using WorkerCommFnType = std::function<void()>;

using WorkUnitCountType = int64_t;
static constexpr WorkUnitCountType const no_work_units = 1;

enum eWorkerGroupEvent {
  WorkersIdle = 1,
  WorkersBusy = 2,
  InvalidEvent = -1
};

#define WORKER_GROUP_EVENT_STR(EVT)                                     \
  EVT == eWorkerGroupEvent::WorkersIdle ? "WorkersIdle" : (             \
    EVT == eWorkerGroupEvent::WorkersBusy ? "WorkersBusy" :  (          \
      EVT == eWorkerGroupEvent::InvalidEvent ? "InvalidEvent" : "Error" \
    )                                                                   \
  )                                                                     \

using WorkerFinishedFnType = std::function<void(WorkerIDType, WorkUnitCountType)>;

}} /* end namespace vt::worker */

#endif /*INCLUDED_WORKER_WORKER_COMMON_H*/
