
#if !defined INCLUDED_MESSAGING_MESSAGE_SHARED_MESSAGE_H
#define INCLUDED_MESSAGING_MESSAGE_SHARED_MESSAGE_H

#include "config.h"
#include "messaging/envelope.h"
#include "pool/pool.h"

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

  debug_print(
    pool, node,
    "messageRef msg={}, refs={}\n",
    print_ptr(msg), envelopeGetRef(msg->env)
  );
}

template <typename MessageT>
void messageDeref(MessageT* msg) {
  envelopeDeref(msg->env);

  debug_print(
    pool, node,
    "messageDeref msg={}, refs={}\n",
    print_ptr(msg), envelopeGetRef(msg->env)
  );

  #if backend_check_enabled(memory_pool) && \
     !backend_check_enabled(no_pool_alloc_env)
  if (envelopeGetRef(msg->env) == 0) {
    thePool()->dealloc(msg);
  }
  #endif
}

} //end namespace vt

#endif /*INCLUDED_MESSAGING_MESSAGE_SHARED_MESSAGE_H*/
