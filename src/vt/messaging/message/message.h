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
#include "vt/messaging/envelope.h"
#include "vt/pool/pool.h"

#include <typeinfo>
#include <type_traits>

namespace vt { namespace messaging {

struct BaseMsg { };

template <typename EnvelopeT>
struct ActiveMsg : BaseMsg {
  using EnvelopeType = EnvelopeT;

  /*
   * Be careful here: `has_owner_' needs to precede the EnvelopeType because
   * this field may be accessed in contexts where the EnvelopeType is not yet
   * checked/determined
   */

  bool has_owner_ = false;
  EnvelopeType env;

  ActiveMsg() {
    envelopeInitEmpty(env);

    debug_print(
      pool, node,
      "Message::constructor of ptr={}, type={}\n",
      print_ptr(this), typeid(this).name()
    );
  }

  #if backend_check_enabled(memory_pool) && \
     !backend_check_enabled(no_pool_alloc_env)
  static void* operator new(std::size_t sz) {
    auto const& ptr = thePool()->alloc(sz);

    debug_print(
      pool, node,
      "Message::new of size={}, ptr={}\n", sz, print_ptr(ptr)
    );

    return ptr;
  }

  static void* operator new(std::size_t sz, std::size_t oversize) {
    auto const& ptr = thePool()->alloc(sz, oversize);

    debug_print(
      pool, node,
      "Message::new (special sized) of size={}, oversize={}, ptr={}\n",
      sz, oversize, print_ptr(ptr)
    );

    return ptr;
  }

  static void operator delete(void* ptr) {
    debug_print(
      pool, node,
      "Message::delete of ptr={}\n", print_ptr(ptr)
    );

    return thePool()->dealloc(ptr);
  }

  static void* operator new(std::size_t, void* mem) {
    return mem;
  }
  #else
  static void* operator new(std::size_t sz) {
    return std::malloc(sz);
  }

  static void* operator new(std::size_t sz, std::size_t oversize) {
    return std::malloc(sz + oversize);
  }

  static void operator delete(void* ptr) {
    std::free(ptr);
  }

  static void* operator new(std::size_t, void* mem) {
    return mem;
  }
  #endif

  // Explicitly write parent serialize so derived messages can contain non-byte
  // serialization. Envelopes, by default, are required to be byte serializable.
  template <typename SerializerT>
  void serializeParent(SerializerT& s) { }

  template <typename SerializerT>
  void serializeThis(SerializerT& s) {
    // @todo: do not serialize the entire envelope---it contains specific data
    // for this message
    s | has_owner_;
    s | env;
  }
};

}} //end namespace vt::messaging

namespace vt {

using BaseMessage     = messaging::BaseMsg;
template <typename EnvelopeT>
using ActiveMessage   = messaging::ActiveMsg<EnvelopeT>;
using ShortMessage    = messaging::ActiveMsg<Envelope>;
using EpochMessage    = messaging::ActiveMsg<EpochEnvelope>;
using EpochTagMessage = messaging::ActiveMsg<EpochTagEnvelope>;
using Message         = EpochTagMessage;

using BaseMsgType     = ShortMessage;

static_assert(
  std::is_trivially_destructible<ShortMessage>::value,
  "ShortMessage must be trivial destructible"
);
static_assert(
  std::is_trivially_destructible<EpochMessage>::value,
  "EpochMessage must be trivial destructible"
);
static_assert(
  std::is_trivially_destructible<EpochTagMessage>::value,
  "EpochTagMessage must be trivial destructible"
);

} // end namespace vt

#endif /*INCLUDED_MESSAGING_MESSAGE_MESSAGE_H*/
