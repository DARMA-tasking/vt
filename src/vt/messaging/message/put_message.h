
#if !defined INCLUDED_MESSAGING_MESSAGE_PUT_MESSAGE_H
#define INCLUDED_MESSAGING_MESSAGE_PUT_MESSAGE_H

#include "config.h"
#include "messaging/envelope.h"
#include "messaging/message/message.h"

namespace vt { namespace messaging {

template <typename MessageT>
struct PutMessageComponent : MessageT {
  void setPut(void const* const ptr, size_t const size) {
    envelopeSetPutPtr(MessageT::env, ptr, size);
  }
  void* getPut() {
    return envelopeGetPutPtr(MessageT::env);
  }
  size_t getPutSize() {
    return envelopeGetPutSize(MessageT::env);
  }
};

}} //end namespace vt::messaging

namespace vt {

using PayloadMessage = messaging::PutMessageComponent<
  ActiveMessage<PutShortEnvelope>
>;

} // end namespace vt

#endif /*INCLUDED_MESSAGING_MESSAGE_PUT_MESSAGE_H*/
