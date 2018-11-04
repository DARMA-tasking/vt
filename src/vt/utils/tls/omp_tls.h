
#if !defined INCLUDED_UTILS_TLS_OMP_TLS_H
#define INCLUDED_UTILS_TLS_OMP_TLS_H

#include "vt/config.h"

#if backend_check_enabled(openmp)
#include <omp.h>

namespace vt { namespace util { namespace tls {

template <typename T, char const* tag, T val>
struct ThreadLocalInitOMP {
  static T& get() { return value_; }
private:
  #if defined(__GNUC__)
    static thread_local T value_;
  #else
    static T value_;
    #pragma omp threadprivate (value_)
  #endif
};

template <typename T, char const* tag>
struct ThreadLocalOMP {
  static T& get() { return value_; }
private:
  #if defined(__GNUC__)
    static thread_local T value_;
  #else
    static T value_;
    #pragma omp threadprivate (value_)
  #endif
};

#if defined(__GNUC__)
  template <typename T, char const* tag, T val>
  T thread_local ThreadLocalInitOMP<T,tag,val>::value_ = val;

  template <typename T, char const* tag>
  T thread_local ThreadLocalOMP<T,tag>::value_;
#else
  template <typename T, char const* tag, T val>
  T ThreadLocalInitOMP<T,tag,val>::value_ = val;

  template <typename T, char const* tag>
  T ThreadLocalOMP<T,tag>::value_;
#endif

}}} /* end namespace vt::util::tls */

#endif /*backend_check_enabled(openmp)*/
#endif /*INCLUDED_UTILS_TLS_OMP_TLS_H*/
