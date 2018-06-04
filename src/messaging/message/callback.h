
#if !defined INCLUDED_MESSAGING_MESSAGE_CALLBACK_H
#define INCLUDED_MESSAGING_MESSAGE_CALLBACK_H

#include "config.h"
#include "messaging/message/message.h"

namespace vt { namespace messaging {

struct CallbackMsg : ::vt::Message {
  CallbackMsg() : Message() {
    setCallbackType(env);
  }

  void setCallback(HandlerType const& han) { callback_ = han; }
  HandlerType getCallback() const { return callback_; }

  // Explicitly write parent serialize so derived messages can contain non-byte
  // serialization. Envelopes, by default, are required to be byte serializable.
  template <typename SerializerT>
  void serializeParent(SerializerT& s) {
    Message::serializeParent(s);
    Message::serializeThis(s);
  }

  template <typename SerializerT>
  void serializeThis(SerializerT& s) {
    s | callback_;
  }

  static inline HandlerType getCallbackMessage(ShortMessage* msg) {
    return reinterpret_cast<CallbackMsg*>(msg)->getCallback();
  }

private:
  HandlerType callback_ = uninitialized_handler;
};

}} /* end namespace vt::messaging */

namespace vt {

using CallbackMessage = messaging::CallbackMsg;

} // end namespace vt

#endif /*INCLUDED_MESSAGING_MESSAGE_CALLBACK_H*/
