
#if !defined INCLUDED_MESSAGING_MESSAGE_SHARED_MESSAGE_IMPL_H
#define INCLUDED_MESSAGING_MESSAGE_SHARED_MESSAGE_IMPL_H

#include "config.h"
#include "messaging/envelope.h"
#include "messaging/message/smart_ptr.h"
#include "pool/pool.h"

namespace vt {

template <typename MsgT, typename... Args>
MsgT* makeSharedMessage(Args&&... args) {
  auto msg = new MsgT{std::forward<Args>(args)...};
  envelopeSetRef(msg->env, 1);
  msg->has_owner_ = false;
  return msg;
}

template <typename MsgT, typename... Args>
MsgSharedPtr<MsgT> makeMessage(Args&&... args) {
  auto msg = makeSharedMessage<MsgT>(std::forward<Args>(args)...);
  return promoteMsgOwner<MsgT>(msg);
}

template <typename MessageT, typename... Args>
MessageT* makeSharedMessageSz(std::size_t size, Args&&... args) {
  MessageT* msg = new (size) MessageT{std::forward<Args>(args)...};
  envelopeSetRef(msg->env, 1);
  return msg;
}

template <typename MessageT>
void messageConvertToShared(MessageT* msg) {
  envelopeSetRef(msg->env, 1);
}

template <typename MessageT>
void messageSetUnmanaged(MessageT* msg) {
  envelopeSetRef(msg->env, not_shared_message);
}

template <typename MsgPtrT>
void messageResetDeserdes(MsgPtrT const& msg) {
  envelopeSetRef(msg->env, 1);
  msg->has_owner_ = true;
}

} //end namespace vt

#endif /*INCLUDED_MESSAGING_MESSAGE_SHARED_MESSAGE_IMPL_H*/
