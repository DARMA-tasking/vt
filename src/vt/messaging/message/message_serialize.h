/*
//@HEADER
// *****************************************************************************
//
//                            message_serialize.h
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_MESSAGING_MESSAGE_MESSAGE_SERIALIZE_H
#define INCLUDED_MESSAGING_MESSAGE_MESSAGE_SERIALIZE_H

#include <type_traits>

#if HAS_SERIALIZATION_LIBRARY

// These should probably be .. elsewhere.
// They are lifted for demonstration purposes.
#define HAS_DETECTION_COMPONENT 1
#include "serialization_library_headers.h"
#include "traits/serializable_traits.h"

namespace vt { namespace messaging {

// Detection support to determine if a message type actually contains
// a serialize method, itself; the checkpoint built-in detection will return
// false positives for types deriving from types with intrusive
// serializtion functions defined.
// ~HOWEVER~
// GCC forces a template instantion when attempting to get the type
// of the specific instantiated template method. Clang will return
// the relevant type information without an unncessary instantiation.
// Checking the serialization method is not required, but it can catch
// some message hierarchy related errors during compilation.
#if defined(__clang__)
#define VT_CHECK_FOR_SERIALIZE_METHOD_ON_TYPE 1

template <typename U, typename = void>
struct has_own_serialize_member_t : std::false_type {};

template <typename U>
struct has_own_serialize_member_t<U,
  std::enable_if_t<
    std::is_same<void (U::*)(::serdes::Sizer&), decltype(&U::template serialize<::serdes::Sizer&>)>::value
  >>
  : std::true_type
{};

// Either declared out-of-line (no inheritence in play)
// or a serialize member is declared on the precise type.
template <typename T>
static constexpr auto const has_own_serialize =
  ::serdes::SerializableTraits<T>::has_serialize_noninstrusive
  or has_own_serialize_member_t<T>::value;
#endif

}} // end vt::messaging

#endif

/** \file */

// These macros are always available, regardless of serialization support.
// (Otherwise they would have to be guarded in non-serialization builds.)

#define vt_msg_serialize_mode_(MODE)                                           \
  void vt_serialize_defined_on_type() {}                                       \
  static constexpr ::vt::messaging::SerializationMode vt_serialize_mode        \
    = ::vt::messaging::SerializationMode::MODE

/**
 * \brief Mark the message as requiring serialization.
 *
 * It is an error if the message does not provide serialization support.
 */
#define vt_msg_serialize_required()                                            \
  vt_msg_serialize_mode_(require);                                             \
  static_assert(                                                               \
    not ::vt::messaging::msg_serialization_mode<MessageParentType>::prohibited, \
    "Message cannot require serialization"                                     \
    " because serialization is prohibited by the parent type."                 \
  )

/**
 * \brief Mark the message as supporting BUT NOT USING serialzation.
 *
 * This message MUST support a serialization function. However, this
 * support is only for derived message usage.
 *
 * The message itself will NOT be sent with serialization.
 * Standard copyable transmission rules apply, even if
 * subtypes of this message may be serialized.
 */
#define vt_msg_serialize_supported()                                           \
  vt_msg_serialize_mode_(support);                                             \
  static_assert(                                                               \
    not ::vt::messaging::msg_serialization_mode<MessageParentType>::required,  \
    "Message must require serialization"                                       \
    " because serialization is required by the parent type."                   \
  );                                                                           \
  static_assert(                                                               \
    not ::vt::messaging::msg_serialization_mode<MessageParentType>::prohibited, \
    "Message must prohibit serialization"                                      \
    " because serialization is prohibited by the parent type."                 \
  )

/**
 * \brief Prohibit the message from having serialization support.
 *
 * The message CANNOT support a serialization function
 * and the message is always sent without serialization.
 * Standard copyable transmission rules apply.
 */
#define vt_msg_serialize_prohibited()                                          \
  vt_msg_serialize_mode_(prohibit);                                            \
  static_assert(                                                               \
    not ::vt::messaging::msg_serialization_mode<MessageParentType>::required,  \
    "Message cannot prohibit serialization"                                    \
    " because serialization is required by the parent type."                   \
  )

/**
 * \brief Mark the message as serializable if needed by the base type.
 *
 * If the message does not need to be serialized it is treated as supporting
 * serialization, even if serialization is prohibited by the base type.
 */
#define vt_msg_serialize_if_needed_by_parent()                                 \
  void vt_serialize_defined_on_type() {}                                       \
  static constexpr ::vt::messaging::SerializationMode vt_serialize_mode        \
  = (::vt::messaging::msg_serialization_mode<MessageParentType>::required      \
     ? ::vt::messaging::SerializationMode::require                             \
     : ::vt::messaging::SerializationMode::support)

/**
 * \brief Mark the message as serializable if needed by the base type
 * or by any of the specified types.
 *
 * If the message does not need to be serialized it is treated as supporting
 * serialization, even if serialization is prohibited by the base type.
 */
#define vt_msg_serialize_if_needed_by_parent_or_type1(TYPE1)                   \
  void vt_serialize_defined_on_type() {}                                       \
  static constexpr ::vt::messaging::SerializationMode vt_serialize_mode        \
  = (::vt::messaging::msg_serialization_mode<MessageParentType>::required      \
     or not (true                                                              \
       and vt::messaging::byte_copyable<TYPE1>::value                          \
     )                                                                         \
     ? ::vt::messaging::SerializationMode::require                             \
     : ::vt::messaging::SerializationMode::support)

#define vt_msg_serialize_if_needed_by_parent_or_type2(TYPE1, TYPE2)            \
  void vt_serialize_defined_on_type() {}                                       \
  static constexpr ::vt::messaging::SerializationMode vt_serialize_mode        \
  = (::vt::messaging::msg_serialization_mode<MessageParentType>::required      \
     or not (true                                                              \
       and vt::messaging::byte_copyable<TYPE1>::value                          \
       and vt::messaging::byte_copyable<TYPE2>::value                          \
     )                                                                         \
     ? ::vt::messaging::SerializationMode::require                             \
     : ::vt::messaging::SerializationMode::support)

#define vt_msg_serialize_if_needed_by_parent_or_type3(TYPE1, TYPE2, TYPE3)     \
  void vt_serialize_defined_on_type() {}                                       \
  static constexpr ::vt::messaging::SerializationMode vt_serialize_mode        \
  = (::vt::messaging::msg_serialization_mode<MessageParentType>::required      \
     or not (true                                                              \
       and vt::messaging::byte_copyable<TYPE1>::value                          \
       and vt::messaging::byte_copyable<TYPE2>::value                          \
       and vt::messaging::byte_copyable<TYPE3>::value                          \
     )                                                                         \
     ? ::vt::messaging::SerializationMode::require                             \
     : ::vt::messaging::SerializationMode::support)

namespace vt { namespace messaging {

enum SerializationMode
{
  not_specified,     // default value
  support,           // this has a serialization function that DERIVED types can use.
                     // this type itself is NOT SERIALIZED for transmission.
  require,           // REQUIRE that the type is serialized
  prohibit           // this type cannot be serialized
};

template <typename T>
struct byte_copyable {
  constexpr static bool value = std::is_trivially_copyable<T>::value and not std::is_pointer<T>::value;
};

// ref. https://en.cppreference.com/w/cpp/types/void_t
template<typename... Ts>
struct cxx14_make_void { typedef void type;};
template<typename... Ts>
using cxx14_void_t = typename cxx14_make_void<Ts...>::type;

// Use member type to detect if the mode is directly specified.
// (Can't figure out how to prevent detection of static constexpr from parent.)
template <typename U, typename = void>
struct msg_defines_serialize_mode
  : std::false_type
{};

template <typename U>
struct msg_defines_serialize_mode<U,
  std::enable_if_t<
    std::is_same<void (U::*)(), decltype(&U::vt_serialize_defined_on_type)>::value
  >>
  : std::true_type
{};

template <typename MessageT, typename = void>
struct msg_serialization_mode {
  static constexpr bool supported = false;
  static constexpr bool required = false;
  static constexpr bool prohibited = false;
};

// Specialization - applies iff static member present
template <typename MessageT>
struct msg_serialization_mode<
  MessageT,
  cxx14_void_t<decltype(MessageT::vt_serialize_mode)>
> {
  static constexpr bool supported
    = MessageT::vt_serialize_mode == SerializationMode::support;
  static constexpr bool required
    = MessageT::vt_serialize_mode == SerializationMode::require;
  static constexpr bool prohibited
    = MessageT::vt_serialize_mode == SerializationMode::prohibit;
};

}} //end namespace vt::messaging

#endif /*INCLUDED_MESSAGING_MESSAGE_MESSAGE_SERIALIZE_H*/
