
#if !defined INCLUDED_SERIALIZATION_AUTO_DISPATCH_DISPATCH_HANDLER_IMPL_H
#define INCLUDED_SERIALIZATION_AUTO_DISPATCH_DISPATCH_HANDLER_IMPL_H

#include "config.h"
#include "serialization/auto_dispatch/dispatch_handler.h"
#include "serialization/serialize_interface.h"
#include "serialization/messaging/serialized_messenger.h"
#include "messaging/active.h"

#include <cassert>

namespace vt { namespace serialization { namespace auto_dispatch {

template <typename MsgT>
/*static*/ EventType SenderHandler<MsgT>::sendMsg(
  NodeType const& node, MsgT* msg, HandlerType const& handler,
  TagType const& tag, ActionType action
) {
  return theMsg()->sendMsg<MsgT>(node,handler,msg,tag,action);
}

template <typename MsgT>
/*static*/ EventType BroadcasterHandler<MsgT>::broadcastMsg(
  MsgT* msg, HandlerType const& handler, TagType const& tag,
  ActionType action
) {
  return theMsg()->broadcastMsg<MsgT>(handler,msg,tag,action);
}

template <typename MsgT>
/*static*/ EventType SenderSerializeHandler<MsgT>::sendMsgParserdes(
  NodeType const& node, HandlerType const& han, MsgT* msg,
  TagType const& tag, ActionType action
) {
  assert(tag == no_tag && "Tagged messages serialized not implemented");
  SerializedMessenger::sendParserdesMsgHandler<MsgT>(node,han,msg);
  // @todo: forward event through chain
  return no_event;
}

template <typename MsgT>
/*static*/ EventType SenderSerializeHandler<MsgT>::sendMsg(
  NodeType const& node, MsgT* msg, HandlerType const& handler,
  TagType const& tag, ActionType action
) {
  assert(tag == no_tag && "Tagged messages serialized not implemented");
  SerializedMessenger::sendSerialMsgHandler<MsgT>(node,msg,handler);
  // @todo: forward event through chain
  return no_event;
}

template <typename MsgT>
/*static*/ EventType
 BroadcasterSerializeHandler<MsgT>::broadcastMsgParserdes(
   MsgT* msg, HandlerType const& handler, TagType const& tag,
   ActionType action
) {
  assert(tag == no_tag && "Tagged messages serialized not implemented");
  SerializedMessenger::broadcastParserdesMsgHandler<MsgT>(msg,handler);
  // @todo: forward event through chain
  return no_event;
}

template <typename MsgT>
/*static*/ EventType BroadcasterSerializeHandler<MsgT>::broadcastMsg(
  MsgT* msg, HandlerType const& handler, TagType const& tag,
  ActionType action
) {
  assert(tag == no_tag && "Tagged messages serialized not implemented");
  SerializedMessenger::broadcastSerialMsgHandler<MsgT>(msg,handler);
  // @todo: forward event through chain
  return no_event;
}


}}} /* end namespace vt::serialization::auto_dispatch */

#endif /*INCLUDED_SERIALIZATION_AUTO_DISPATCH_DISPATCH_HANDLER_IMPL_H*/
