
#if ! defined __RUNTIME_TRANSPORT_UTILS_CONCURRENT_DEQUE__
#define __RUNTIME_TRANSPORT_UTILS_CONCURRENT_DEQUE__

#include "config.h"
#include "mutex.h"
#include "concurrent_deque_locked.h"

#if backend_check_enabled(openmp)
#include <omp.h>
#include "omp_mutex.h"
#endif

#include <mutex>

namespace vt { namespace util { namespace container {

#if backend_check_enabled(openmp)
  template <typename T>
  using ConcurrentDequeOMP = ConcurrentDequeLocked<T, mutex::OMPMutex>;
#else
  template <typename T>
  using ConcurrentDequeSTD = ConcurrentDequeLocked<T, std::mutex>;
#endif

}}} //end namespace vt::util::container

namespace vt { namespace util { namespace container {

template <typename T>
using ConcurrentDeque = ConcurrentDequeLocked<T, vt::util::mutex::MutexType>;

}}} //end namespace vt::util::container

#endif /*__RUNTIME_TRANSPORT_UTILS_CONCURRENT_DEQUE__*/

