
#if !defined INCLUDED_UTILS_TLS_STD_TLS_H
#define INCLUDED_UTILS_TLS_STD_TLS_H

#include "vt/config.h"

#if backend_check_enabled(stdthread)

namespace vt { namespace util { namespace tls {

template <typename T, char const* tag, T val = T{}>
struct ThreadLocalInitSTD {
  static T& get() { return value_; }
private:
  static thread_local T value_;
};

template <typename T, char const* tag>
struct ThreadLocalSTD {
  static T& get() { return value_; }
private:
  static thread_local T value_;
};

template <typename T, char const* tag, T val>
thread_local T ThreadLocalInitSTD<T,tag,val>::value_ = {val};

template <typename T, char const* tag>
thread_local T ThreadLocalSTD<T,tag>::value_;

}}} /* end namespace vt::util::tls */

#endif /*backend_check_enabled(stdthread)*/

#endif /*INCLUDED_UTILS_TLS_STD_TLS_H*/
