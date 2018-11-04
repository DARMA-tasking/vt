
#if !defined INCLUDED_UTILS_STATIC_CHECKS_ALL_TRUE_H
#define INCLUDED_UTILS_STATIC_CHECKS_ALL_TRUE_H

#include "vt/config.h"

namespace vt { namespace util {

template <bool...> struct BoolPack;
template <bool... bs>
using all_true = std::is_same<BoolPack<bs..., true>, BoolPack<true, bs...>>;

}} /* end namespace vt::util */

#endif /*INCLUDED_UTILS_STATIC_CHECKS/ALL_TRUE_H*/
