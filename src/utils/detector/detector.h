
#if ! defined __RUNTIME_TRANSPORT_UTILS_DETECTOR__
#define __RUNTIME_TRANSPORT_UTILS_DETECTOR__

#include "config.h"
#include "utils/detector/void_t.h"
#include "utils/detector/none_such.h"

#include <type_traits>
#include <utility>

#if backend_check_enabled(detector)

namespace vt { namespace util { namespace detection {

/*
 * Implementation of the standard C++ detector idiom, used to statically
 * detect/assert properties of C++ types.
 */
template <typename T, typename, template <typename...> class Op, typename... Args>
struct detector {
  constexpr static auto value = false;
  using value_t = std::false_type;
  using type = T;
};

template <typename T, template <typename...> class Op, typename... Args>
struct detector<T, void_t<Op<Args...>>, Op, Args...>  {
  constexpr static auto value = true;
  using value_t = std::true_type;
  using type = Op<Args...>;
};

template <template <typename...> class Op, typename... Args>
using is_detected = detector<NoneSuchType, void, Op, Args...>;

template <template <typename...> class Op, typename... Args>
using detected_t = typename is_detected<Op, Args...>::type;

template <typename ExpectedT, template<typename...> class Op, typename... Args>
using is_detected_exact = std::is_same<detected_t<Op, Args...>, ExpectedT>;

template <typename T, template <typename...> class Op, typename... Args>
using is_detected_convertible = std::is_convertible<detected_t<Op, Args...>, T>;

}}}  // end vt::util::detection

#endif /* backend_check_enabled(detector) */

#endif /*__RUNTIME_TRANSPORT_UTILS_DETECTOR__*/
