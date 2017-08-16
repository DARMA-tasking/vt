
#if ! defined __RUNTIME_TRANSPORT_MESSAGE__
#define __RUNTIME_TRANSPORT_MESSAGE__

#include "common.h"
#include "envelope.h"

namespace runtime {

struct BaseMessage {
  using envelope_t = Envelope;
  envelope_t env;

  BaseMessage() {
    envelope_init_empty(env);
  }
};

struct EpochMessage {
  using envelope_t = EpochEnvelope;
  envelope_t env;

  EpochMessage() {
    envelope_init_empty(env);
  }
};

struct TagMessage {
  using envelope_t = EpochEnvelope;
  envelope_t env;

  TagMessage() {
    envelope_init_empty(env);
  }
};

struct EpochTagMessage {
  using envelope_t = EpochTagEnvelope;
  envelope_t env;

  EpochTagMessage() {
    envelope_init_empty(env);
  }
};

// default runtime::Message includes tag and epoch
using Message = EpochTagMessage;

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_MESSAGE__*/
