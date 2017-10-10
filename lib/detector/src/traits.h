
#if !defined INCLUDED_DETECTOR_LIB_BASIC_TRAITS
#define INCLUDED_DETECTOR_LIB_BASIC_TRAITS

#include "detector_common.h"
#include "detector.h"

#include <type_traits>
#include <utility>

namespace detection {

template <typename T>
struct BasicTraits {
  template <typename U, typename... Vs>
  using constructor_t = decltype(U(std::declval<Vs>()...));
  using has_default_constructor = detection::is_detected<constructor_t, T>;

  template <typename U>
  using copy_constructor_t = decltype(U(std::declval<U const&>()));
  using has_copy_constructor = detection::is_detected<copy_constructor_t, T>;

  template <typename U>
  using equality_t = decltype(std::declval<U>().operator==(std::declval<U const&>()));
  using has_equality_op = detection::is_detected<equality_t, T>;
};

}  // end detection

#endif /*INCLUDED_DETECTOR_LIB_BASIC_TRAITS*/
