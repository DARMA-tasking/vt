
#if !defined INCLUDED_MESSAGING_MESSAGE_MESSAGE_H
#define INCLUDED_MESSAGING_MESSAGE_MESSAGE_H

#include "config.h"
#include "messaging/envelope.h"
#include "messaging/message/shared_message.h"
#include "pool/pool.h"

#include <typeinfo>

namespace vt { namespace messaging {

struct BaseMsg { };

template <typename EnvelopeT>
struct ActiveMsg : BaseMsg {
  using EnvelopeType = EnvelopeT;
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

} // end namespace vt

#endif /*INCLUDED_MESSAGING_MESSAGE_MESSAGE_H*/
