
#if !defined __RUNTIME_TRANSPORT_WORKER_TYPES__
#define __RUNTIME_TRANSPORT_WORKER_TYPES__

#include "config.h"

#include <functional>

namespace vt { namespace worker {

using WorkUnitType = std::function<void()>;

}} /* end namespace vt::worker */

#endif /*__RUNTIME_TRANSPORT_WORKER_TYPES__*/
