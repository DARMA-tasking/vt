
#if !defined INCLUDED_UTILS_TLS_NULL_TLS_H
#define INCLUDED_UTILS_TLS_NULL_TLS_H

#include "config.h"

#if backend_no_threading

namespace vt { namespace util { namespace tls {

template <typename T, char const* tag, T val = T{}>
struct ThreadLocalNull {
  using TypeT = T;

  static T& get() { return value_; }

private:
  static T value_;
};

template <typename T, char const* tag, T val>
typename ThreadLocalNull<T,tag,val>::TypeT ThreadLocalNull<T,tag,val>::value_ = {val};

}}} /* end namespace vt::util::tls */

#endif /*backend_no_threading*/

#endif /*INCLUDED_UTILS_TLS_NULL_TLS_H*/
