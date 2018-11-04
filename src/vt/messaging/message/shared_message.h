
#if !defined INCLUDED_MESSAGING_MESSAGE_SHARED_MESSAGE_H
#define INCLUDED_MESSAGING_MESSAGE_SHARED_MESSAGE_H

#include "vt/config.h"
#include "vt/messaging/envelope.h"
#include "vt/pool/pool.h"

namespace vt {

template <typename MessageT, typename... Args>
MessageT* makeSharedMessage(Args&&... args) {
  MessageT* msg = new MessageT{std::forward<Args>(args)...};
  envelopeSetRef(msg->env, 1);
  return msg;
}

template <typename MessageT, typename... Args>
MessageT* makeSharedMessageSz(std::size_t size, Args&&... args) {
  MessageT* msg = new (size) MessageT{std::forward<Args>(args)...};
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
    // @todo: what is the correct strategy here? invoking dealloc does not
    // invoke the destructor
    //
    // thePool()->dealloc(msg);
    //
    delete msg;
  }
  #endif
}

} //end namespace vt

#endif /*INCLUDED_MESSAGING_MESSAGE_SHARED_MESSAGE_H*/
