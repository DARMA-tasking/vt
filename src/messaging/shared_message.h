
#if !defined INCLUDED_MESSAGING_SHARED_MESSAGE_H
#define INCLUDED_MESSAGING_SHARED_MESSAGE_H

#include "config.h"
#include "messaging/envelope.h"

namespace vt {

template <typename MessageT, typename... Args>
MessageT* makeSharedMessage(Args&&... args) {
  MessageT* msg = new MessageT{std::forward<Args>(args)...};
  envelopeSetRef(msg->env, 1);
  return msg;
}

template <typename MessageT>
bool isSharedMessage(MessageT* msg) {
  return envelopeGetRef(msg->env) != not_shared_message;
}

template <typename MessageT>
void messageConvertToShared(MessageT* msg) {
  envelopeSetRef(msg->env, 1);
}

template <typename MessageT>
void messageSetUnmanaged(MessageT* msg) {
  envelopeSetRef(msg->env, not_shared_message);
}

template <typename MessageT>
void messageRef(MessageT* msg) {
  envelopeRef(msg->env);
}

template <typename MessageT>
void messageDeref(MessageT* msg) {
  envelopeDeref(msg->env);

  debug_print(
    pool, node,
    "messageDeref msg=%p, refs=%d\n", msg, envelopeGetRef(msg->env)
  );

  if (envelopeGetRef(msg->env) == 0) {
    thePool->dealloc(msg);
  }
}

} //end namespace vt

#endif /*INCLUDED_MESSAGING_SHARED_MESSAGE_H*/
