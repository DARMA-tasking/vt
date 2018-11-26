/*
//@HEADER
// ************************************************************************
//
//                          traits.h
//                                VT
//              Copyright (C) 2017 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_TOPOS_INDEX_TRAITS_TRAITS_H
#define INCLUDED_TOPOS_INDEX_TRAITS_TRAITS_H

#include "vt/config.h"

#include <cstdint>
#include <functional>
#include <string>

#if backend_check_enabled(detector)
  #include "detector_headers.h"
#endif /* backend_check_enabled(detector) */

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
  using copy_assignment_t = decltype(
    std::declval<U>().operator=(std::declval<U const&>())
  );
  using has_copy_assignment = detection::is_detected_convertible<
    T, copy_assignment_t, T
  >;

  template <typename U>
  using operator_eq_t = decltype(
    U(std::declval<U>().operator=(std::declval<U const&>()))
  );
  using has_operator_eq = detection::is_detected<operator_eq_t, T>;

  template <typename U>
  using equality_t = decltype(
    std::declval<U>().operator==(std::declval<U const&>())
  );
  using has_equality = detection::is_detected<equality_t, T>;

  template <typename U>
  using packedSize_t = decltype(std::declval<U const&>().packedSize());
  using has_packedSize = detection::is_detected_convertible<
    size_t, packedSize_t, T
  >;

  template <typename U>
  using indexIsByteCopyable_t = decltype(
    std::declval<U const&>().indexIsByteCopyable()
  );
  using has_indexIsByteCopyable = detection::is_detected<
    indexIsByteCopyable_t, T
  >;

  template <typename U>
  using uniqueBits_t = decltype(std::declval<U const&>().uniqueBits());
  using has_uniqueBits = detection::is_detected_convertible<
    UniqueIndexBitType, uniqueBits_t, T
  >;

  template <typename U>
  using ApplyType = std::function<void(U)>;
  template <typename U>
  using foreach_t = decltype(
    std::declval<U const&>().foreach(
      std::declval<U const&>(), std::declval<ApplyType<U>>()
    ));
  using has_foreach = detection::is_detected<foreach_t, T>;

  template <typename U>
  using IsByteCopyable_t = typename U::IsByteCopyable;
  using has_IsByteCopyable = detection::is_detected<IsByteCopyable_t, T>;

  template <typename U>
  using toString_t = decltype(std::declval<U const&>().toString());
  using has_toString = detection::is_detected_convertible<
    std::string, toString_t, T
  >;

  /*
   * This defines what it means to be an `Index'; i.e., the index concept.
   */
  static constexpr auto const is_index =
    /*
     *  default constructor, copy constructor, copy assignment operator
     */
    has_copy_constructor::value    and
    has_default_constructor::value and
    has_copy_assignment::value     and
    /*
     *  operator==
     */
    has_equality::value            and
    has_operator_eq::value         and
    /*
     *  typedefs:
     *    IndexSizeType, IsByteCopyable
     */
    has_IndexSizeType::value       and
    has_IsByteCopyable::value      and
    /*
     * methods:
     *   packedSize(), indexIsByteCopyable(), uniqueBits(), foreach(),
     *   toString()
     */
    has_packedSize::value          and
    has_indexIsByteCopyable::value and
    has_uniqueBits::value          and
    has_foreach::value             and
    has_toString::value;
};

}}  // end namespace vt::index

#endif  /*backend_check_enabled(detector)*/

#endif /*INCLUDED_TOPOS_INDEX_TRAITS_TRAITS_H*/
