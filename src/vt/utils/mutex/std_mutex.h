
#if !defined INCLUDED_UTILS_MUTEX_STD_MUTEX_H
#define INCLUDED_UTILS_MUTEX_STD_MUTEX_H

#include "vt/config.h"

#if backend_check_enabled(stdthread)
#include <mutex>

namespace vt { namespace util { namespace mutex {

using STDMutex = std::mutex;

}}} // end namespace vt::util::mutex

#if backend_check_enabled(detector)
  #include "vt/utils/mutex/mutex_traits.h"

  namespace vt { namespace util { namespace mutex {

  static_assert(
    MutexTraits<STDMutex>::is_mutex,
    "STDMutex should follow the mutex concept"
  );

  }}} // end namespace vt::util::mutex
#endif

#endif /*backend_check_enabled(stdthread)*/

#endif /*INCLUDED_UTILS_MUTEX_STD_MUTEX_H*/
