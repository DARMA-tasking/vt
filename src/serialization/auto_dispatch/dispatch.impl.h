
#if !defined INCLUDED_SERIALIZATION_AUTO_DISPATCH_DISPATCH_IMPL_H
#define INCLUDED_SERIALIZATION_AUTO_DISPATCH_DISPATCH_IMPL_H

#include "config.h"
#include "serialization/auto_dispatch/dispatch.h"
#include "serialization/serialize_interface.h"
#include "serialization/messaging/serialized_messenger.h"
#include "messaging/active.h"

#include <cassert>

namespace vt { namespace serialization { namespace auto_dispatch {

/*
 * Regular message sned/braoadcast pass-though (acts as delegate)
 */
template <typename MessageT, ActiveTypedFnType<MessageT>* f>
/*static*/ EventType Sender<MessageT,f>::sendMsg(
  NodeType const& node, MessageT* msg, TagType const& tag, ActionType action
) {
  return theMsg()->sendMsg<MessageT,f>(node,msg,tag,action);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
/*static*/ EventType Broadcaster<MessageT,f>::broadcastMsg(
  MessageT* msg, TagType const& tag, ActionType action
) {
  return theMsg()->broadcastMsg<MessageT,f>(msg,tag,action);
}

/*
 * Serialization message sned/braoadcast detected based on the is_serializable
 * type traits
 */
template <typename MessageT, ActiveTypedFnType<MessageT>* f>
/*static*/ EventType SenderSerialize<MessageT,f>::sendMsg(
  NodeType const& node, MessageT* msg, TagType const& tag, ActionType action
) {
  assert(tag == no_tag && "Tagged messages serialized not implemented");
  SerializedMessenger::sendSerialMsg<MessageT,f>(node,msg);
  // @todo: forward event through chain
  return no_event;
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
/*static*/ EventType BroadcasterSerialize<MessageT,f>::broadcastMsg(
  MessageT* msg, TagType const& tag, ActionType action
) {
  assert(tag == no_tag && "Tagged messages serialized not implemented");
  ::fmt::print("calling broadcaster serialize!!!!!!!!!!!\n");
  SerializedMessenger::broadcastSerialMsg<MessageT,f>(msg);
  // @todo: forward event through chain
  return no_event;
}

}}} /* end namespace vt::serialization::auto_dispatch */

#endif /*INCLUDED_SERIALIZATION_AUTO_DISPATCH_DISPATCH_IMPL_H*/
