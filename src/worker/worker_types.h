
#if !defined INCLUDED_WORKER_WORKER_TYPES_H
#define INCLUDED_WORKER_WORKER_TYPES_H

#include "config.h"

#include <functional>

namespace vt { namespace worker {

using WorkUnitType = std::function<void()>;

}} /* end namespace vt::worker */

#endif /*INCLUDED_WORKER_WORKER_TYPES_H*/
