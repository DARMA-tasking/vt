
#if ! defined __RUNTIME_TRANSPORT_MESSAGE__
#define __RUNTIME_TRANSPORT_MESSAGE__

#include "common.h"
#include "envelope.h"

namespace runtime {

struct BaseMessage { };

template <typename EnvelopeT>
struct ActiveMessage : BaseMessage {
  using envelope_t = EnvelopeT;
  envelope_t env;

  ActiveMessage() {
    envelope_init_empty(env);
  }
};

using ShortMessage = ActiveMessage<Envelope>;
using EpochMessage = ActiveMessage<EpochEnvelope>;
using EpochTagMessage = ActiveMessage<EpochTagEnvelope>;

// default runtime::Message includes tag and epoch
using Message = EpochTagMessage;

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_MESSAGE__*/
