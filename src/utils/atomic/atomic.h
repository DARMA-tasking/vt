
#if !defined INCLUDED_UTILS_ATOMIC_ATOMIC_H
#define INCLUDED_UTILS_ATOMIC_ATOMIC_H

#include "config.h"

#if backend_check_enabled(openmp)
  #include "utils/atomic/omp_atomic.h"
#elif backend_check_enabled(stdthread)
  #include "utils/atomic/std_atomic.h"
#endif

namespace vt { namespace util { namespace atomic {

#if backend_check_enabled(openmp)
  template <typename T>
  using AtomicType = AtomicOMP<T>;
#else
  template <typename T>
  using AtomicType = AtomicSTD<T>;
#endif

}}} /* end namespace vt::util::atomic */

#endif /*INCLUDED_UTILS_ATOMIC_ATOMIC_H*/
