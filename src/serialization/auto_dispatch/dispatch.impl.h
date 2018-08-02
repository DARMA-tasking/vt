
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
template <typename MsgT, ActiveTypedFnType<MsgT>* f>
/*static*/ EventType Sender<MsgT,f>::sendMsg(
  NodeType const& node, MsgT* msg, TagType const& tag, ActionType action
) {
  return theMsg()->sendMsg<MsgT,f>(node,msg,tag,action);
}

template <typename MsgT>
/*static*/ EventType SenderHandler<MsgT>::sendMsgHandler(
  NodeType const& node, MsgT* msg, HandlerType const& handler,
  TagType const& tag, ActionType action
) {
  return theMsg()->sendMsg<MsgT>(node,handler,msg,tag,action);
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
/*static*/ EventType Broadcaster<MsgT,f>::broadcastMsg(
  MsgT* msg, TagType const& tag, ActionType action
) {
  return theMsg()->broadcastMsg<MsgT,f>(msg,tag,action);
}

template <typename MsgT>
/*static*/ EventType BroadcasterHandler<MsgT>::broadcastMsgHandler(
  MsgT* msg, HandlerType const& handler, TagType const& tag,
  ActionType action
) {
  return theMsg()->broadcastMsg<MsgT>(msg,handler,tag,action);
}

/*
 * Serialization message sned/braoadcast detected based on the is_serializable
 * type traits
 */
template <typename MsgT, ActiveTypedFnType<MsgT>* f>
/*static*/ EventType SenderSerialize<MsgT,f>::sendMsgParserdes(
  NodeType const& node, MsgT* msg, TagType const& tag, ActionType action
) {
  assert(tag == no_tag && "Tagged messages serialized not implemented");
  SerializedMessenger::sendParserdesMsg<MsgT,f>(node,msg);
  // @todo: forward event through chain
  return no_event;
}

template <typename MsgT>
/*static*/ EventType SenderSerializeHandler<MsgT>::sendMsgParserdesHandler(
  NodeType const& node, HandlerType const& han, MsgT* msg,
  TagType const& tag, ActionType action
) {
  assert(tag == no_tag && "Tagged messages serialized not implemented");
  SerializedMessenger::sendParserdesMsgHandler<MsgT>(node,han,msg);
  // @todo: forward event through chain
  return no_event;
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
/*static*/ EventType SenderSerialize<MsgT,f>::sendMsg(
  NodeType const& node, MsgT* msg, TagType const& tag, ActionType action
) {
  assert(tag == no_tag && "Tagged messages serialized not implemented");
  SerializedMessenger::sendSerialMsg<MsgT,f>(node,msg);
  // @todo: forward event through chain
  return no_event;
}

template <typename MsgT>
/*static*/ EventType SenderSerializeHandler<MsgT>::sendMsgHandler(
  NodeType const& node, MsgT* msg, HandlerType const& handler,
  TagType const& tag, ActionType action
) {
  assert(tag == no_tag && "Tagged messages serialized not implemented");
  SerializedMessenger::sendSerialMsgHandler<MsgT>(node,msg,handler);
  // @todo: forward event through chain
  return no_event;
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
/*static*/ EventType BroadcasterSerialize<MsgT,f>::broadcastMsgParserdes(
  MsgT* msg, TagType const& tag, ActionType action
) {
  assert(tag == no_tag && "Tagged messages serialized not implemented");
  SerializedMessenger::broadcastParserdesMsg<MsgT,f>(msg);
  // @todo: forward event through chain
  return no_event;
}

template <typename MsgT>
/*static*/ EventType
 BroadcasterSerializeHandler<MsgT>::broadcastMsgParserdesHandler(
   MsgT* msg, HandlerType const& handler, TagType const& tag,
   ActionType action
) {
  assert(tag == no_tag && "Tagged messages serialized not implemented");
  SerializedMessenger::broadcastParserdesMsgHandler<MsgT>(msg,handler);
  // @todo: forward event through chain
  return no_event;
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
/*static*/ EventType BroadcasterSerialize<MsgT,f>::broadcastMsg(
  MsgT* msg, TagType const& tag, ActionType action
) {
  assert(tag == no_tag && "Tagged messages serialized not implemented");
  SerializedMessenger::broadcastSerialMsg<MsgT,f>(msg);
  // @todo: forward event through chain
  return no_event;
}

template <typename MsgT>
/*static*/ EventType BroadcasterSerializeHandler<MsgT>::broadcastMsgHandler(
  MsgT* msg, HandlerType const& handler, TagType const& tag,
  ActionType action
) {
  assert(tag == no_tag && "Tagged messages serialized not implemented");
  SerializedMessenger::broadcastSerialMsgHandler<MsgT>(msg,handler);
  // @todo: forward event through chain
  return no_event;
}

}}} /* end namespace vt::serialization::auto_dispatch */

#endif /*INCLUDED_SERIALIZATION_AUTO_DISPATCH_DISPATCH_IMPL_H*/
