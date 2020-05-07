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

// These should probably be .. elsewhere.
// They are lifted for demonstration purposes.
#include <checkpoint/checkpoint.h>

namespace vt { namespace messaging {

// Only some compilers can directly check for a serialization method
// because to do so requires declval on a templated member WITHOUT
// causing templated instantiation (which will fail).
#ifndef vt_quirked_serialize_method_detection

template <typename U, typename = void>
struct has_own_serialize_member_t : std::false_type {};

template <typename U>
struct has_own_serialize_member_t<U,
  std::enable_if_t<
    std::is_same<
      void (U::*)(::checkpoint::Sizer&),
      decltype(&U::template serialize<::checkpoint::Sizer&>)
    >::value
  >>
  : std::true_type
{};

// Either declared out-of-line (no inheritence in play)
// or a serialize member is declared on the precise type.
template <typename T>
static constexpr auto const has_own_serialize =
  ::checkpoint::SerializableTraits<T>::has_serialize_noninstrusive
  or has_own_serialize_member_t<T>::value;

#endif

}} // end vt::messaging

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
       and vt::messaging::is_byte_copyable_t<TYPE1>::value                     \
     )                                                                         \
     ? ::vt::messaging::SerializationMode::require                             \
     : ::vt::messaging::SerializationMode::support)

#define vt_msg_serialize_if_needed_by_parent_or_type2(TYPE1, TYPE2)            \
  void vt_serialize_defined_on_type() {}                                       \
  static constexpr ::vt::messaging::SerializationMode vt_serialize_mode        \
  = (::vt::messaging::msg_serialization_mode<MessageParentType>::required      \
     or not (true                                                              \
       and vt::messaging::is_byte_copyable_t<TYPE1>::value                     \
       and vt::messaging::is_byte_copyable_t<TYPE2>::value                     \
     )                                                                         \
     ? ::vt::messaging::SerializationMode::require                             \
     : ::vt::messaging::SerializationMode::support)

#define vt_msg_serialize_if_needed_by_parent_or_type3(TYPE1, TYPE2, TYPE3)     \
  void vt_serialize_defined_on_type() {}                                       \
  static constexpr ::vt::messaging::SerializationMode vt_serialize_mode        \
  = (::vt::messaging::msg_serialization_mode<MessageParentType>::required      \
     or not (true                                                              \
       and vt::messaging::is_byte_copyable_t<TYPE1>::value                     \
       and vt::messaging::is_byte_copyable_t<TYPE2>::value                     \
       and vt::messaging::is_byte_copyable_t<TYPE3>::value                     \
     )                                                                         \
     ? ::vt::messaging::SerializationMode::require                             \
     : ::vt::messaging::SerializationMode::support)

namespace vt { namespace messaging {

enum struct SerializationMode : int
{
 support = 1,
 require = 2,
 prohibit = 3
};

// Trivial test to ensure that the type is eligible to be byte-copied,
// regardless of if the message supports explicit serialization.
// N.B. This will return TRUE on certain compilers even though the guarantee
// cannot be validated due to implementation quirks of trivially copyable.
#ifndef vt_quirked_trivially_copyable_on_msg
template <typename T>
struct is_byte_copyable_t {
  constexpr static bool value = std::is_trivially_copyable<T>::value and not std::is_pointer<T>::value;
};
#else
template <typename T>
struct is_byte_copyable_t {
  // Although this can detect some cases it is not strictly true.
  constexpr static bool value = std::is_trivially_destructible<T>::value and not std::is_pointer<T>::value;
};
#endif

// C++17 port. ref. https://en.cppreference.com/w/cpp/types/conjunction
template<class...> struct cxx14_conjunction : std::true_type { };
template<class B1> struct cxx14_conjunction<B1> : B1 { };
template<class B1, class... Bn>
struct cxx14_conjunction<B1, Bn...>
  : std::conditional_t<bool(B1::value), cxx14_conjunction<Bn...>, B1> {};

// C++17 port. ref. https://en.cppreference.com/w/cpp/types/void_t
template<typename... Ts>
struct cxx14_make_void { typedef void type;};
template<typename... Ts>
using cxx14_void_t = typename cxx14_make_void<Ts...>::type;

// Foward-declare
struct BaseMsg;

// Allows testing of is_base_of<DefinesSerializationMode<Msg>,Msg> to determine
// if one of the CRTP types is used, as the templated type is invariant.
// All of the CRTP types are required to define the serialize mode which will
// be picked up via normal inheritance.
template <typename MsgT>
struct DefinesSerializationMode {
};

template <typename MsgT, typename SelfT>
struct NonSerializedMsg : MsgT, DefinesSerializationMode<SelfT>
{
  template <typename... Args>
  NonSerializedMsg(Args&&... args)
    : MsgT{std::forward<Args>(args)...}
  {
  }

  static constexpr ::vt::messaging::SerializationMode vt_serialize_mode
    = ::vt::messaging::SerializationMode::prohibit;

  static_assert(
    std::is_base_of<BaseMsg, MsgT>::value,
    "Message must derive from BaseMsg."
  );

  static_assert(
    MsgT::vt_serialize_mode not_eq vt::messaging::SerializationMode::require,
    "Parent message must not require serialization."
  );

  static_assert(
    vt::messaging::is_byte_copyable_t<MsgT>::value,
    "Message must be byte-copyable because serialization is prohibited."
  );

  // Attempts to call serialize(..) are forbidden on types expressly
  // prohibited from being serialized. Assumes CRTP call not skipped
  // in derived type to be an effective guard. Active Message is also
  // expected to apply it's own static assert as a cover.
  template <typename SerializerT>
  void serialize(SerializerT& s) = delete;
};

template <typename MsgT, typename SelfT>
struct SerializeSupportedMsg : MsgT, DefinesSerializationMode<SelfT>
{
  template <typename... Args>
  SerializeSupportedMsg(Args&&... args)
    : MsgT{std::forward<Args>(args)...}
  {
  }

  static constexpr ::vt::messaging::SerializationMode vt_serialize_mode
    = ::vt::messaging::SerializationMode::support;

  static_assert(
    std::is_base_of<BaseMsg, MsgT>::value,
    "Message must derive from BaseMsg."
  );

  static_assert(
    MsgT::vt_serialize_mode == vt::messaging::SerializationMode::support,
    "Message supporting serialization must derive from a message"
    " that also supports serialization."
  );

  static_assert(
    vt::messaging::is_byte_copyable_t<MsgT>::value,
    "Message must be byte-copyable because serialization may be"
    " prohibited in a derived type."
  );

  template <typename SerializerT>
  inline void serialize(SerializerT& s) {
    MsgT::serialize(s);
  }
};

template <typename MsgT, typename SelfT>
struct SerializeRequiredMsg : MsgT, DefinesSerializationMode<SelfT>
{
  template <typename... Args>
  SerializeRequiredMsg(Args&&... args)
    : MsgT{std::forward<Args>(args)...}
  {
  }

  static constexpr ::vt::messaging::SerializationMode vt_serialize_mode
    = ::vt::messaging::SerializationMode::require;

  static_assert(
    std::is_base_of<BaseMsg, MsgT>::value,
    "Message must derive from BaseMsg."
  );

  static_assert(
    MsgT::vt_serialize_mode not_eq vt::messaging::SerializationMode::prohibit,
    "Message requiring serialization cannot derive from a message"
    " that prohibits serialization."
  );

  template <typename SerializerT>
  inline void serialize(SerializerT& s) {
    MsgT::serialize(s);
  }
};

template <typename MsgT, typename SelfT, typename ...DepTypesT>
struct SerializeIfNeededMsg : MsgT, DefinesSerializationMode<SelfT>
{
  template <typename... Args>
  SerializeIfNeededMsg(Args&&... args)
    : MsgT{std::forward<Args>(args)...}
  {
  }

  static constexpr ::vt::messaging::SerializationMode vt_serialize_mode
  = MsgT::vt_serialize_mode == SerializationMode::require
    or not cxx14_conjunction<is_byte_copyable_t<DepTypesT>...>::value
    ? SerializationMode::require
    : SerializationMode::support;

  static_assert(
    std::is_base_of<BaseMsg, MsgT>::value,
    "Message must derive from BaseMsg."
  );

  static_assert(false
    or (vt_serialize_mode == SerializationMode::support
        and MsgT::vt_serialize_mode == vt::messaging::SerializationMode::support)
    or (vt_serialize_mode == SerializationMode::require
        and MsgT::vt_serialize_mode not_eq vt::messaging::SerializationMode::prohibit),
    "If serialization is not required, base message must support serialization."
    " If serialization is required, base message must not prohibit serialization."
  );

  template <typename SerializerT>
  inline void serialize(SerializerT& s) {
    MsgT::serialize(s);
  }
};

// Use member type to detect if the mode is directly specified.
// (Can't figure out how to prevent detection of static constexpr from parent.)
template <typename U, typename = void>
struct msg_defines_serialize_mode
  : std::false_type
{};

// For messages built with macros
template <typename U>
struct msg_defines_serialize_mode<U,
  std::enable_if_t<
    std::is_same<void (U::*)(), decltype(&U::vt_serialize_defined_on_type)>::value
  >>
  : std::true_type
{};

// For messages using CRTP (inherited one level in CRTP-derived types)
template <typename U>
struct msg_defines_serialize_mode<U,
  std::enable_if_t<
    std::is_base_of<DefinesSerializationMode<U>,U>::value
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


// Export CRTP types
namespace vt {

template <typename MsgT, typename SelfT>
using NonSerialized = vt::messaging::NonSerializedMsg<MsgT, SelfT>;

template <typename MsgT, typename SelfT>
using SerializeSupported = vt::messaging::SerializeSupportedMsg<MsgT, SelfT>;

template <typename MsgT, typename SelfT>
using SerializeRequired = vt::messaging::SerializeRequiredMsg<MsgT, SelfT>;

template <typename MsgT, typename SelfT, typename ...DepTypesT>
using SerializeIfNeeded = vt::messaging::SerializeIfNeededMsg<MsgT, SelfT, DepTypesT...>;

} //end namespace vt

#endif /*INCLUDED_MESSAGING_MESSAGE_MESSAGE_SERIALIZE_H*/
