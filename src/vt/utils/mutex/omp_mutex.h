
#if !defined INCLUDED_UTILS_MUTEX_OMP_MUTEX_H
#define INCLUDED_UTILS_MUTEX_OMP_MUTEX_H

#include "vt/config.h"

#if backend_check_enabled(openmp)

#include <omp.h>

namespace vt { namespace util { namespace mutex {

struct OMPMutex {
  omp_lock_t omp_lock;

  OMPMutex();
  OMPMutex(OMPMutex const&) = delete;

  virtual ~OMPMutex();

  void lock();
  void unlock();
  bool try_lock();
};

}}} /* end namespace vt::util::mutex */

#if backend_check_enabled(detector)
  #include "vt/utils/mutex/mutex_traits.h"

  namespace vt { namespace util { namespace mutex {

  static_assert(
    MutexTraits<OMPMutex>::is_mutex,
    "OMPMutex must follow the mutex concept"
  );

  }}} // end namespace vt::util::mutex
#endif

#endif

#endif /*INCLUDED_UTILS_MUTEX_OMP_MUTEX_H*/
