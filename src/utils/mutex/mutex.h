
#if !defined INCLUDED_UTILS_MUTEX_MUTEX_H
#define INCLUDED_UTILS_MUTEX_MUTEX_H

#include "config.h"

#include <mutex>

#if backend_check_enabled(openmp)
  #include "utils/mutex/omp_mutex.h"
#elif backend_check_enabled(stdthread)
  #include "utils/mutex/std_mutex.h"
#elif backend_no_threading
  #include "utils/mutex/null_mutex.h"
#else
  backend_static_assert_unreachable
#endif

namespace vt { namespace util { namespace mutex {

#if backend_check_enabled(openmp)
  using MutexType = OMPMutex;
#elif backend_check_enabled(stdthread)
  using MutexType = STDMutex;
#elif backend_no_threading
  using MutexType = NullMutex;
#else
  backend_static_assert_unreachable
#endif

}}} /* end namespace vt::util::mutex */

#endif /*INCLUDED_UTILS_MUTEX_MUTEX_H*/
