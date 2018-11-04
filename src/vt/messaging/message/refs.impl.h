
#if !defined INCLUDED_MESSAGING_MESSAGE_REFS_IMPL_H
#define INCLUDED_MESSAGING_MESSAGE_REFS_IMPL_H

#include "vt/config.h"
#include "vt/messaging/envelope.h"
#include "vt/context/context.h"

namespace vt {

template <typename MessageT>
void messageRef(MessageT* msg) {
  envelopeRef(msg->env);

  debug_print(
    pool, node,
    "messageRef msg={}, refs={}\n",
    print_ptr(msg), envelopeGetRef(msg->env)
  );
}

template <template <typename> class MsgPtrT, typename MsgT>
void messageRef(MsgPtrT<MsgT> msg) {
  return messageRef(msg.get());
}

template <typename MessageT>
void messageDeref(MessageT* msg) {
  envelopeDeref(msg->env);

  debug_print(
    pool, node,
    "messageDeref msg={}, refs={}\n",
    print_ptr(msg), envelopeGetRef(msg->env)
  );

  if (envelopeGetRef(msg->env) == 0) {
    /* @todo: what is the correct strategy here? invoking dealloc does not
     * invoke the destructor
     *
     * Instead of explicit/direct dealloc to pool:
     *   thePool()->dealloc(msg);
     *
     * Call `delete' to trigger the destructor and execute overloaded
     * functionality! This should trigger the pool dealloc invocation if it is
     * being used.
     *
     */
    delete msg;
  }
}

template <template <typename> class MsgPtrT, typename MsgT>
void messageDeref(MsgPtrT<MsgT> msg) {
  return messageDeref(msg.get());
}

template <typename MessageT>
bool isSharedMessage(MessageT* msg) {
  return envelopeGetRef(msg->env) != not_shared_message;
}

} /* end namespace vt */

#endif /*INCLUDED_MESSAGING_MESSAGE_REFS_IMPL_H*/
