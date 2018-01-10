
#if !defined INCLUDED_DETECTOR_LIB_DETECTOR
#define INCLUDED_DETECTOR_LIB_DETECTOR

#include "detector_common.h"
#include "void_t.h"
#include "none_such.h"

#include <type_traits>
#include <utility>
#include <tuple>

namespace detection {

/*
 * Implementation of the standard C++ detector idiom, used to statically
 * detect/assert properties of C++ types.
 */
template <typename T, typename, template <typename...> class Op, typename... Args>
struct detector {
  constexpr static auto value = false;
  using value_t = std::false_type;
  using type = T;
  using element_type = T;
};

template <typename T, template <typename...> class Op, typename... Args>
struct detector<T, void_t<Op<Args...>>, Op, Args...>  {
  constexpr static auto value = true;
  using value_t = std::true_type;
  using type = Op<Args...>;
  using element_type = typename std::tuple_element<0, std::tuple<Args...>>::type;
};

template <template <typename...> class Op, typename... Args>
using is_detected = detector<NoneSuchType, void, Op, Args...>;

template <template <typename...> class Op, typename... Args>
using detected_t = typename is_detected<Op, Args...>::type;

template <typename ExpectedT, template<typename...> class Op, typename... Args>
using is_detected_exact = std::is_same<detected_t<Op, Args...>, ExpectedT>;

template <typename T, template <typename...> class Op, typename... Args>
using is_detected_convertible = std::is_convertible<detected_t<Op, Args...>, T>;

}  // end detection

#endif /*INCLUDED_DETECTOR_LIB_DETECTOR*/
