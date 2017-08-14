
#if ! defined __RUNTIME_TRANSPORT_MESSAGE__
#define __RUNTIME_TRANSPORT_MESSAGE__

#include "common.h"
#include "envelope.h"

namespace runtime {

struct Message {
  using envelope_t = Envelope;

  envelope_t env;

  Message() = default;
};

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_MESSAGE__*/
