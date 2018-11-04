
#if !defined INCLUDED_MESSAGING_MESSAGE_SHARED_MESSAGE_H
#define INCLUDED_MESSAGING_MESSAGE_SHARED_MESSAGE_H

#include "vt/config.h"
#include "vt/messaging/envelope.h"
#include "vt/messaging/message/smart_ptr.h"
#include "vt/pool/pool.h"

namespace vt {

template <typename MessageT, typename... Args>
MessageT* makeSharedMessage(Args&&... args);

template <typename MsgT, typename... Args>
MsgSharedPtr<MsgT> makeMessage(Args&&... args);

template <typename MessageT, typename... Args>
MessageT* makeSharedMessageSz(std::size_t size, Args&&... args);

template <typename MsgT, typename... Args>
MsgSharedPtr<MsgT> makeMessageSz(std::size_t size, Args&&... args);

template <typename MessageT>
void messageConvertToShared(MessageT* msg);

template <typename MessageT>
void messageSetUnmanaged(MessageT* msg);

template <typename MsgPtrT>
void messageResetDeserdes(MsgPtrT const& msg);

} //end namespace vt

#include "messaging/message/shared_message.impl.h"

#endif /*INCLUDED_MESSAGING_MESSAGE_SHARED_MESSAGE_H*/
