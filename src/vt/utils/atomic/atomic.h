
#if !defined INCLUDED_UTILS_ATOMIC_ATOMIC_H
#define INCLUDED_UTILS_ATOMIC_ATOMIC_H

#include "config.h"

#if backend_check_enabled(openmp)
  #include "utils/atomic/omp_atomic.h"
#elif backend_check_enabled(stdthread)
  #include "utils/atomic/std_atomic.h"
#elif backend_no_threading
  #include "utils/atomic/null_atomic.h"
#else
  backend_static_assert_unreachable
#endif

namespace vt { namespace util { namespace atomic {

#if backend_check_enabled(openmp)
  template <typename T>
  using AtomicType = AtomicOMP<T>;
#elif backend_check_enabled(stdthread)
  template <typename T>
  using AtomicType = AtomicSTD<T>;
#elif backend_no_threading
  template <typename T>
  using AtomicType = AtomicNull<T>;
#else
  backend_static_assert_unreachable
#endif

}}} /* end namespace vt::util::atomic */

#endif /*INCLUDED_UTILS_ATOMIC_ATOMIC_H*/
