
#if !defined INCLUDED_UTILS_TLS_OMP_TLS_H
#define INCLUDED_UTILS_TLS_OMP_TLS_H

#include "config.h"

#if backend_check_enabled(openmp)
#include <omp.h>

namespace vt { namespace util { namespace tls {

template <typename T, char const* tag, T val = T{}>
struct ThreadLocalOMP {
  using TypeT = T;

  static T& get() { return value_; }

private:
  static T value_;
  #pragma omp threadprivate (value_)
};

template <typename T, char const* tag, T val>
typename ThreadLocalOMP<T,tag,val>::TypeT ThreadLocalOMP<T,tag,val>::value_ = {val};

}}} /* end namespace vt::util::tls */

#endif /*backend_check_enabled(openmp)*/
#endif /*INCLUDED_UTILS_TLS_OMP_TLS_H*/
