
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
      "Message::constructor of ptr=%p, type=%s\n",
      this, typeid(this).name()
    );
  }

  static void* operator new(std::size_t sz) {
    auto const& ptr = thePool()->alloc(sz);

    debug_print(
      pool, node,
      "Message::new of size=%lu, ptr=%p\n", sz, ptr
    );

    return ptr;
  }

  static void* operator new(std::size_t, void* mem) {
    return mem;
  }

  static void operator delete(void* ptr) {
    debug_print(
      pool, node,
      "Message::delete of ptr=%p\n", ptr
    );

    return thePool()->dealloc(ptr);
  }

  // Explicitly write serialize so derived messages can contain non-byte
  // serialization. Envelopes, by default, are required to be byte serializable.
  template <typename SerializerT>
  void serialize(SerializerT& s) {
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

} // end namespace vt

#endif /*INCLUDED_MESSAGING_MESSAGE_MESSAGE_H*/
