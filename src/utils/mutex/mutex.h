
#if !defined __RUNTIME_TRANSPORT_MUTEX__
#define __RUNTIME_TRANSPORT_MUTEX__

#include "config.h"

#include <mutex>

#if backend_check_enabled(openmp)
#include <omp.h>
#include "omp_mutex.h"
#else
#include "std_mutex.h"
#endif

namespace vt { namespace util { namespace mutex {

#if backend_check_enabled(openmp)
  using MutexType = OMPMutex;
#else
  using MutexType = std::mutex;
#endif

}}} /* end namespace vt::util::mutex */

#endif /*__RUNTIME_TRANSPORT_MUTEX__*/
