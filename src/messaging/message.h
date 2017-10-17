
#if !defined INCLUDED_MESSAGING_MESSAGE_H
#define INCLUDED_MESSAGING_MESSAGE_H

#include "config.h"
#include "messaging/envelope.h"
#include "pool/pool.h"
#include "shared_message.h"

namespace vt {

struct BaseMessage { };

template <typename EnvelopeT>
struct ActiveMessage : BaseMessage {
  using EnvelopeType = EnvelopeT;
  EnvelopeType env;

  ActiveMessage() {
    envelopeInitEmpty(env);
  }

  static void* operator new(std::size_t sz) {
    return thePool()->alloc(sz);
  }

  static void* operator new(std::size_t, void* mem) {
    return mem;
  }

  static void operator delete(void* ptr) {
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

#endif /*INCLUDED_MESSAGING_MESSAGE_H*/
