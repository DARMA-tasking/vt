/*
//@HEADER
// *****************************************************************************
//
//                                  message.h
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

#if !defined INCLUDED_MESSAGING_MESSAGE_MESSAGE_H
#define INCLUDED_MESSAGING_MESSAGE_MESSAGE_H

#include "vt/config.h"
#include "vt/pool/pool.h"

#include "vt/messaging/envelope.h"
#include "vt/messaging/message/message_serialize.h"

#include <typeinfo>
#include <type_traits>

namespace vt { namespace messaging {

/** \file */

/**
 * \struct BaseMsg message.h vt/messaging/message/message.h
 *
 * \brief The very lowest base class for a message. Used by the runtime to cast
 * on delivery to handlers without type knowledge.
 */
struct BaseMsg { };

/**
 * \struct ActiveMsg message.h vt/messaging/message/message.h
 *
 * \brief The base class for all messages. Common alias is \c vt::Message which
 * uses the default envelope.
 *
 * This is templated over the envelope to allow longer or shorter messages to be
 * used depending on the target and needed envelope bits.
 *
 */
template <typename EnvelopeT>
struct ActiveMsg : BaseMsg {
  using EnvelopeType = EnvelopeT;

  /*
   * \internal
   * \brief The envelope metadata for the message.
   */
  // n.b. Should be first member even if can't guarantee standard layout.
  EnvelopeType env;

  /**
   * \brief Construct an empty message; initializes the envelope state.
   */
  ActiveMsg() {
    // This is here for legacy reasons (which current allow detection of when
    // a message has not been created correctly). With just this base
    // setup, a vtAssert will fail when the message is attempted to be sent.
    // Proper initialization happens in 'makeMessage' calls.
    envelopeInitEmpty(env);

    vt_debug_print(
      verbose, pool,
      "Message::constructor of ptr={}, type={}\n",
      print_ptr(this), typeid(this).name()
    );
  }

  #if vt_check_enabled(memory_pool) && \
     !vt_check_enabled(no_pool_alloc_env)
  /**
   * \brief Overload of the new operator to use the memory pool to construct a
   * new message
   *
   * \param[in] sz the size to allocate
   *
   * \return a pointer to the allocated memory
   */
  static void* operator new(std::size_t sz) {
    auto const& ptr = thePool()->alloc(sz);

    vt_debug_print(
      verbose, pool,
      "Message::new of size={}, ptr={}\n", sz, print_ptr(ptr)
    );

    return ptr;
  }

  /**
   * \brief Overload of the new operator to use the memory pool to construct a
   * new message with some extra memory after the message for a put payload.
   *
   * \param[in] sz the message size to allocate
   * \param[in] oversize the extra size to allocate
   *
   * \return a pointer to the allocated memory
   */
  static void* operator new(std::size_t sz, std::size_t oversize) {
    auto const& ptr = thePool()->alloc(sz, oversize);

    vt_debug_print(
      verbose, pool,
      "Message::new (special sized) of size={}, oversize={}, ptr={}\n",
      sz, oversize, print_ptr(ptr)
    );

    return ptr;
  }

  /**
   * \brief Overload of delete to use the memory pool to de-allocate a message
   *
   * \param[in] ptr the pointer to deallocate
   */
  static void operator delete(void* ptr) {
    vt_debug_print(
      verbose, pool,
      "Message::delete of ptr={}\n", print_ptr(ptr)
    );

    return thePool()->dealloc(ptr);
  }

  /**
   * \brief Overload of the new operator for in-place allocations
   *
   * \param[in] size_t the size to allocate
   * \param[in] mem the memory to in-place allocation
   *
   * \return the pointer
   */
  static void* operator new(std::size_t, void* mem) {
    vt_debug_print(
      verbose, pool,
      "Message::new (in-place) of ptr={}\n", print_ptr(mem)
    );

    return mem;
  }
  #else
  static void* operator new(std::size_t sz) {
    auto ptr = std::malloc(sz);

    vt_debug_print(
      verbose, gen,
      "Message::new (malloc) of size={}, ptr={}\n", sz, print_ptr(ptr)
    );

    return ptr;
  }

  static void* operator new(std::size_t sz, std::size_t oversize) {
    auto ptr = std::malloc(sz + oversize);

    vt_debug_print(
      verbose, gen,
      "Message::new (malloc) of size={}, oversize={}, ptr={}\n",
      sz, oversize, print_ptr(ptr)
    );

    return ptr;
  }

  static void operator delete(void* ptr) {
    vt_debug_print(
      verbose, gen,
      "Message::delete (free) ptr={}\n", print_ptr(ptr)
    );

    std::free(ptr);
  }

  static void* operator new(std::size_t, void* mem) {
    vt_debug_print(
      verbose, gen,
      "Message::new (in-place) ptr={}\n", print_ptr(mem)
    );

    return mem;
  }
  #endif

  /**
   * \brief Explicitly serialize this message.
   *
   * Should be called froms derived type that support/require serialization.
   *
   * \param[in] s the serializer
   */
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    envelopeSetHasBeenSerialized(env, true);
    // There is some (pipe) code that currently requires the envelope to
    // be serialized for correct operation; this may be fixed, in which
    // case it might be possible to omit envelope serialization here.
    s | env;
  }

public:
  // Message supports serialization for derived types.
  // However, only types that REQUIRE serialization will actually
  // be serialized while others use byte-copy transmission.
  using MessageParentType = BaseMsg;
  vt_msg_serialize_mode_(support);
};

}} //end namespace vt::messaging

namespace vt {

/// Alias to the base of all messages
using BaseMessage     = messaging::BaseMsg;
/// Alias to the a message with any envelope
template <typename EnvelopeT>
using ActiveMessage   = messaging::ActiveMsg<EnvelopeT>;
/// Alias to the shortest message available with no epoch or tag allowed
using ShortMessage    = messaging::ActiveMsg<Envelope>;
/// Alias to a message with only an epoch
using EpochMessage    = messaging::ActiveMsg<EpochEnvelope>;
/// Alias to a message with an epoch and tag
using EpochTagMessage = messaging::ActiveMsg<EpochTagEnvelope>;
/// Alias to the default message (with an epoch and tag)
using Message         = EpochTagMessage;

using BaseMsgType     = ShortMessage;

static_assert(
  std::is_trivially_copyable<ShortMessage>::value
  and std::is_trivially_destructible<ShortMessage>::value,
  "ShortMessage must be trivially copyable destructible"
);
static_assert(
  std::is_trivially_copyable<EpochMessage>::value
  and std::is_trivially_destructible<EpochMessage>::value,
  "EpochMessage must be trivial destructible"
);
static_assert(
  std::is_trivially_copyable<EpochTagMessage>::value
  and std::is_trivially_destructible<EpochTagMessage>::value,
  "EpochTagMessage must be trivial destructible"
);

} // end namespace vt

#endif /*INCLUDED_MESSAGING_MESSAGE_MESSAGE_H*/
