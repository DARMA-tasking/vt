
#if !defined __RUNTIME_TRANSPORT_WORKER_COMMON__
#define __RUNTIME_TRANSPORT_WORKER_COMMON__

#include "config.h"

#include <cstdint>

namespace vt { namespace worker {

static constexpr WorkerCountType const num_default_workers = 4;
static constexpr WorkerCountType const num_default_comm = 1;

using WorkerIDType = int32_t;

static constexpr WorkerIDType const no_worker_id = -1;

}} /* end namespace vt::worker */

#endif /*__RUNTIME_TRANSPORT_WORKER_COMMON__*/
