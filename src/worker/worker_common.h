
#if !defined INCLUDED_WORKER_WORKER_COMMON_H
#define INCLUDED_WORKER_WORKER_COMMON_H

#include "config.h"

#include <cstdint>
#include <functional>

namespace vt { namespace worker {

static constexpr WorkerCountType const num_default_workers = 4;
static constexpr WorkerCountType const num_default_comm = 1;

using WorkerCommFnType = std::function<void()>;

}} /* end namespace vt::worker */

#endif /*INCLUDED_WORKER_WORKER_COMMON_H*/
