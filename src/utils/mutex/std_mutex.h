
#if !defined INCLUDED_UTILS_MUTEX_STD_MUTEX_H
#define INCLUDED_UTILS_MUTEX_STD_MUTEX_H

#include "config.h"

#include <mutex>

#if backend_check_enabled(detector)
  #include "mutex_traits.h"

  namespace vt { namespace util { namespace mutex {

  static_assert(
    MutexTraits<std::mutex>::is_mutex,
    "std::mutex should follow the mutex concept"
  );

  }}} // end namespace vt::util::mutex
#endif

#endif /*INCLUDED_UTILS_MUTEX_STD_MUTEX_H*/
