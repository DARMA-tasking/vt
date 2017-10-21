
#if !defined INCLUDED_UTILS_MUTEX_MUTEX_H
#define INCLUDED_UTILS_MUTEX_MUTEX_H

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

#endif /*INCLUDED_UTILS_MUTEX_MUTEX_H*/
