
#if ! defined __RUNTIME_TRANSPORT_MESSAGE__
#define __RUNTIME_TRANSPORT_MESSAGE__

#include "common.h"
#include "envelope.h"
#include "pool.h"

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
    return thePool->alloc(sz);
  }

  static void operator delete(void* ptr) {
    return thePool->dealloc(ptr);
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

  void setCallback(HandlerType const& han) {
    callback = han;
  }

  HandlerType callback = uninitialized_handler;
};

inline HandlerType getCallbackMessage(ShortMessage* msg) {
  return reinterpret_cast<CallbackMessage*>(msg)->callback;
}

} //end namespace vt

#endif /*__RUNTIME_TRANSPORT_MESSAGE__*/
