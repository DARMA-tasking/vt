
#if ! defined __RUNTIME_TRANSPORT_UTILS_VOID_T__
#define __RUNTIME_TRANSPORT_UTILS_VOID_T__

#include "config.h"

#if backend_check_enabled(detector)

namespace vt { namespace util { namespace detection {

/*
 * Implementation of the `void_t' idiom commonly used for the detector
 * pattern. This will exist as std::void_t<...> in C++17, but until then it must
 * be implemented.
 */

template <typename... >
struct MakeVoid {
  using type = void;
};

template <typename... T>
using void_t = typename MakeVoid<T...>::type;

}}}  // end vt::util::detection

#endif /* backend_check_enabled(detector) */

#endif /*__RUNTIME_TRANSPORT_UTILS_VOID_T__*/
