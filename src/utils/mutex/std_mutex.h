
#if !defined __RUNTIME_TRANSPORT_STD_MUTEX__
#define __RUNTIME_TRANSPORT_STD_MUTEX__

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

#endif /*__RUNTIME_TRANSPORT_STD_MUTEX__*/
