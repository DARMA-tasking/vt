
#if !defined INCLUDED_UTILS_STATIC_CHECKS_META_TYPE_EQ__H
#define INCLUDED_UTILS_STATIC_CHECKS_META_TYPE_EQ__H

#include "vt/config.h"
#include "vt/utils/static_checks/cond_.h"

#include <type_traits>

namespace vt { namespace util {

template <typename... Ts>
struct meta_and : std::true_type { };

template <typename T, typename... Ts>
struct meta_and<T, Ts...>
  : cond_<T::value, meta_and<Ts...>, std::false_type>::type
{ };

template <typename T, typename... Ts>
using meta_type_eq = meta_and<std::is_same<Ts, T>...>;

}} /* end namespace vt::util */

#endif /*INCLUDED_UTILS_STATIC_CHECKS_META_TYPE_EQ__H*/
