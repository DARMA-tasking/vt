
#if !defined INCLUDED_UTILS_STATIC_CHECKS_COND__H
#define INCLUDED_UTILS_STATIC_CHECKS_COND__H

#include "vt/config.h"

namespace vt { namespace util {

template <bool B, typename T, typename F>
struct cond_ { using type = T; };

template <typename T, typename F>
struct cond_<false, T, F> { using type = F; };

}} /* end namespace vt::util */

#endif /*INCLUDED_UTILS_STATIC_CHECKS_COND__H*/
