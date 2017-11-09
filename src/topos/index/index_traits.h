
#if !defined INCLUDED_TOPOS_INDEX_TRAITS
#define INCLUDED_TOPOS_INDEX_TRAITS

#include <cstdint>

#include "config.h"

#if backend_check_enabled(detector)
#include "detector_headers.h"
#endif  /*backend_check_enabled(detector)*/

#if backend_check_enabled(detector)

namespace vt { namespace index {

template <typename T>
struct IndexTraits {
  template <typename U>
  using IndexSizeType_t = typename U::IndexSizeType;
  using has_IndexSizeType = detection::is_detected<IndexSizeType_t, T>;

  template <typename U, typename... Vs>
  using constructor_t = decltype(U(std::declval<Vs>()...));
  using has_default_constructor = detection::is_detected<constructor_t, T>;

  template <typename U>
  using copy_constructor_t = decltype(U(std::declval<U const&>()));
  using has_copy_constructor = detection::is_detected<copy_constructor_t, T>;

  template <typename U>
  using operator_eq_t = decltype(U(std::declval<U>().operator=(
                                     std::declval<U const&>()
                                   )));
  using has_operator_eq = detection::is_detected<operator_eq_t, T>;

  template <typename U>
  using equality_t = decltype(std::declval<U>().operator==(std::declval<U const&>()));
  using has_equality = detection::is_detected<equality_t, T>;

  template <typename U>
  using packedSize_t = decltype(std::declval<U const&>().packedSize());
  using has_packedSize = detection::is_detected_convertible<size_t, packedSize_t, T>;

  template <typename U>
  using indexIsByteCopyable_t = decltype(std::declval<U const&>().indexIsByteCopyable());
  using has_indexIsByteCopyable = detection::is_detected<indexIsByteCopyable_t, T>;

  template <typename U>
  using uniqueBits_t = decltype(std::declval<U const&>().uniqueBits());
  using has_uniqueBits = detection::is_detected_convertible<
    UniqueIndexBitType, uniqueBits_t, T
  >;

  // This defines what it means to be an `Index'
  static constexpr auto const is_index =
    // default constructor and copy constructor
    has_copy_constructor::value and has_default_constructor::value and
    // operator ==
    has_equality::value and has_operator_eq::value and
    // typedefs/using IndexSizeType
    has_IndexSizeType::value and
    // methods: packedSize() and indexIsByteCopyable() and uniqueBits()
    has_packedSize::value and has_indexIsByteCopyable::value and has_uniqueBits::value;
};

}}  // end namespace vt::index

#endif  /*backend_check_enabled(detector)*/

#endif  /*INCLUDED_TOPOS_INDEX_TRAITS*/
