
#if !defined INCLUDED_MESSAGING_MESSAGE_SHARED_MESSAGE_IMPL_H
#define INCLUDED_MESSAGING_MESSAGE_SHARED_MESSAGE_IMPL_H

#include "vt/config.h"
#include "vt/messaging/envelope.h"
#include "vt/messaging/message/smart_ptr.h"
#include "vt/pool/pool.h"

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
  msg->has_owner_ = false;
  return msg;
}

template <typename MsgT, typename... Args>
MsgSharedPtr<MsgT> makeMessageSz(std::size_t size, Args&&... args) {
  auto msg = makeSharedMessageSz<MsgT>(size,std::forward<Args>(args)...);
  return promoteMsgOwner<MsgT>(msg);
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
