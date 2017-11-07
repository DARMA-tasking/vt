
#if !defined INCLUDED_UTILS_TLS_NULL_TLS_H
#define INCLUDED_UTILS_TLS_NULL_TLS_H

#include "config.h"

#if backend_no_threading || backend_null_tls

namespace vt { namespace util { namespace tls {

template <typename T, char const* tag>
struct ThreadLocalNull {
  static T& get() { return value_; }
private:
  static T value_;
};

template <typename T, char const* tag, T val>
struct ThreadLocalInitNull {
  static T& get() { return value_; }
private:
  static T value_;
};

template <typename T, char const* tag>
T ThreadLocalNull<T,tag>::value_;

template <typename T, char const* tag, T val>
T ThreadLocalInitNull<T,tag,val>::value_ = {val};

}}} /* end namespace vt::util::tls */

#endif /*backend_no_threading*/

#endif /*INCLUDED_UTILS_TLS_NULL_TLS_H*/
