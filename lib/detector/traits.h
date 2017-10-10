
#if ! defined __RUNTIME_TRANSPORT_UTILS_TRAITS__
#define __RUNTIME_TRANSPORT_UTILS_TRAITS__

#include "config.h"
#include "utils/detector/detector.h"

#include <type_traits>
#include <utility>

#if backend_check_enabled(detector)

namespace vt { namespace util { namespace traits {

using namespace detection;

template <typename T>
struct BasicTraits {
  template <typename U, typename... Vs>
  using constructor_t = decltype(U(std::declval<Vs>()...));

  template <typename U>
  using copy_constructor_t = decltype(U(std::declval<U const&>()));

  template <typename U>
  using equality_t = decltype(std::declval<U>().operator==(std::declval<U const&>()));
};

}}}  // end vt::util::traits

#endif /* backend_check_enabled(detector) */

#endif /*__RUNTIME_TRANSPORT_UTILS_TRAITS__*/
