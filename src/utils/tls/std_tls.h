
#if !defined INCLUDED_UTILS_TLS_STD_TLS_H
#define INCLUDED_UTILS_TLS_STD_TLS_H

#include "config.h"

namespace vt { namespace util { namespace tls {

template <typename T, char const* tag, T val = T{}>
struct ThreadLocalSTD {
  using TypeT = T;

  static T& get() { return value_; }

private:
  static thread_local T value_;
};

template <typename T, char const* tag, T val>
thread_local
typename ThreadLocalSTD<T,tag,val>::TypeT ThreadLocalSTD<T,tag,val>::value_ = {val};

}}} /* end namespace vt::util::tls */

#endif /*INCLUDED_UTILS_TLS_STD_TLS_H*/
