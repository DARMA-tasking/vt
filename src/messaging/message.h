
#if ! defined __RUNTIME_TRANSPORT_MESSAGE__
#define __RUNTIME_TRANSPORT_MESSAGE__

#include "common.h"
#include "envelope.h"
#include "pool.h"

namespace runtime {

struct BaseMessage { };

template <typename EnvelopeT>
struct ActiveMessage : BaseMessage {
  using envelope_t = EnvelopeT;
  envelope_t env;

  ActiveMessage() {
    envelope_init_empty(env);
  }

  static void*
  operator new(std::size_t sz) {
    return the_pool->alloc(sz);
  }

  static void
  operator delete(void* ptr) {
    return the_pool->dealloc(ptr);
  }
};

using ShortMessage = ActiveMessage<Envelope>;
using EpochMessage = ActiveMessage<EpochEnvelope>;
using EpochTagMessage = ActiveMessage<EpochTagEnvelope>;

// default runtime::Message includes tag and epoch
using Message = EpochTagMessage;

struct CallbackMessage : runtime::Message {
  CallbackMessage() : Message() {
    set_callback_type(env);
  }

  void
  set_callback(handler_t const& han) {
    callback = han;
  }

  handler_t callback = uninitialized_handler;
};

inline handler_t
get_callback_message(ShortMessage* msg) {
  return reinterpret_cast<CallbackMessage*>(msg)->callback;
}

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_MESSAGE__*/
