
#if !defined INCLUDED_UTILS_DEMANGLE_GET_TYPE_NAME_H
#define INCLUDED_UTILS_DEMANGLE_GET_TYPE_NAME_H

#include "vt/config.h"
#include "vt/utils/string/static.h"

namespace vt { namespace util { namespace demangle {

using StaticString = util::string::StatStr;

template <class T>
constexpr StaticString type_name() {
#if defined(__clang__)
  StaticString p = __PRETTY_FUNCTION__;
  return StaticString(p.data() + 31, p.size() - 31 - 1);

# elif defined(__GNUC__)
  StaticString p = __PRETTY_FUNCTION__;

#   if __cplusplus < 201402
    return StaticString(p.data() + 36, p.size() - 36 - 1);
#   else
    return StaticString(p.data() + 46, p.size() - 46 - 1);
#   endif

# elif defined(_MSC_VER)
  StaticString p = __FUNCSIG__;
  return StaticString(p.data() + 38, p.size() - 38 - 7);
#endif
}

}}} /* end namespace vt::util::demangle */

#endif /*INCLUDED_UTILS_DEMANGLE_GET_TYPE_NAME_H*/
