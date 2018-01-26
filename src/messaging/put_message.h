
#if !defined INCLUDED_MESSAGING_PUT_MESSAGE_H
#define INCLUDED_MESSAGING_PUT_MESSAGE_H

#include "config.h"
#include "messaging/envelope.h"
#include "messaging/payload.h"
#include "messaging/message.h"

namespace vt {

template <typename MessageT>
struct PutMessageComponent : MessageT {
  void setPut(void const* const ptr, size_t const size) {
    envelopeSetPutPtr(MessageT::env, ptr, size);
  }
  void* getPut() {
    return envelopeGetPutPtr(MessageT::env);
  }
};

using PayloadMessage = PutMessageComponent<ActiveMessage<PutShortEnvelope>>;

} /* end namespace vt */

#endif /*INCLUDED_MESSAGING_PUT_MESSAGE_H*/
