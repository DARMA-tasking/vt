
#if !defined INCLUDED_MESSAGING_MESSAGE_MESSAGE_H
#define INCLUDED_MESSAGING_MESSAGE_MESSAGE_H

#include "config.h"
#include "messaging/envelope.h"
#include "messaging/message/shared_message.h"
#include "pool/pool.h"

#include <typeinfo>

namespace vt {

struct BaseMessage { };

template <typename EnvelopeT>
struct ActiveMessage : BaseMessage {
  using EnvelopeType = EnvelopeT;
  EnvelopeType env;

  ActiveMessage() {
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

using ShortMessage = ActiveMessage<Envelope>;
using EpochMessage = ActiveMessage<EpochEnvelope>;
using EpochTagMessage = ActiveMessage<EpochTagEnvelope>;

// default vt::Message includes tag and epoch
using Message = EpochTagMessage;

struct CallbackMessage : vt::Message {
  CallbackMessage() : Message() {
    setCallbackType(env);
  }

  void setCallback(HandlerType const& han) { callback_ = han; }
  HandlerType getCallback() const { return callback_; }

  // Explicitly write serialize so derived messages can contain non-byte
  // serialization. Envelopes, by default, are required to be byte serializable.
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    Message::serialize(s);
    s | callback_;
  }

  static inline HandlerType getCallbackMessage(ShortMessage* msg) {
    return reinterpret_cast<CallbackMessage*>(msg)->getCallback();
  }

private:
  HandlerType callback_ = uninitialized_handler;
};

} //end namespace vt

#endif /*INCLUDED_MESSAGING_MESSAGE_MESSAGE_H*/
