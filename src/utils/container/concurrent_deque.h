
#if !defined INCLUDED_UTILS_CONTAINER_CONCURRENT_DEQUE_H
#define INCLUDED_UTILS_CONTAINER_CONCURRENT_DEQUE_H

#include "config.h"
#include "utils/mutex/mutex.h"
#include "concurrent_deque_locked.h"

#if backend_check_enabled(openmp)
  #include "utils/mutex/omp_mutex.h"
#elif backend_check_enabled(stdthread)
  #include "utils/mutex/std_mutex.h"
#endif

#include <mutex>

namespace vt { namespace util { namespace container {

#if backend_check_enabled(openmp)
  template <typename T>
  using ConcurrentDequeOMP = ConcurrentDequeLocked<T, mutex::OMPMutex>;
#elif backend_check_enabled(stdthread)
  template <typename T>
  using ConcurrentDequeSTD = ConcurrentDequeLocked<T, std::mutex>;
#endif

}}} //end namespace vt::util::container

namespace vt { namespace util { namespace container {

template <typename T>
using ConcurrentDeque = ConcurrentDequeLocked<T, vt::util::mutex::MutexType>;

}}} //end namespace vt::util::container

#endif /*INCLUDED_UTILS_CONTAINER_CONCURRENT_DEQUE_H*/

