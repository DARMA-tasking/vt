
#if !defined INCLUDED_SERIALIZATION_AUTO_DISPATCH_DISPATCH_FUNCTOR_IMPL_H
#define INCLUDED_SERIALIZATION_AUTO_DISPATCH_DISPATCH_FUNCTOR_IMPL_H

#include "config.h"
#include "serialization/auto_dispatch/dispatch_functor.h"
#include "serialization/serialize_interface.h"
#include "serialization/messaging/serialized_messenger.h"
#include "messaging/active.h"

#include <cassert>

namespace vt { namespace serialization { namespace auto_dispatch {

/*
 * Regular message sned/braoadcast pass-though (acts as delegate)
 */
template <typename FunctorT, typename MsgT>
/*static*/ EventType SenderFunctor<FunctorT,MsgT>::sendMsg(
  NodeType const& node, MsgT* msg, TagType const& tag, ActionType action
) {
  return theMsg()->sendMsg<FunctorT,MsgT>(node,msg,tag,action);
}

template <typename FunctorT, typename MsgT>
/*static*/ EventType BroadcasterFunctor<FunctorT,MsgT>::broadcastMsg(
  MsgT* msg, TagType const& tag, ActionType action
) {
  return theMsg()->broadcastMsg<FunctorT,MsgT>(msg,tag,action);
}

/*
 * Serialization message sned/braoadcast detected based on the is_serializable
 * type traits
 */
template <typename FunctorT, typename MsgT>
/*static*/ EventType SenderSerializeFunctor<FunctorT,MsgT>::sendMsgParserdes(
  NodeType const& node, MsgT* msg, TagType const& tag, ActionType action
) {
  vtAssert(tag == no_tag, "Tagged messages serialized not implemented");
  SerializedMessenger::sendParserdesMsg<FunctorT,MsgT>(node,msg);
  // @todo: forward event through chain
  return no_event;
}

template <typename FunctorT, typename MsgT>
/*static*/ EventType SenderSerializeFunctor<FunctorT,MsgT>::sendMsg(
  NodeType const& node, MsgT* msg, TagType const& tag, ActionType action
) {
  vtAssert(tag == no_tag, "Tagged messages serialized not implemented");
  SerializedMessenger::sendSerialMsg<FunctorT,MsgT>(node,msg);
  // @todo: forward event through chain
  return no_event;
}


template <typename FunctorT, typename MsgT>
/*static*/ EventType
 BroadcasterSerializeFunctor<FunctorT,MsgT>::broadcastMsgParserdes(
  MsgT* msg, TagType const& tag, ActionType action
) {
  vtAssert(tag == no_tag, "Tagged messages serialized not implemented");
  SerializedMessenger::broadcastParserdesMsg<FunctorT,MsgT>(msg);
  // @todo: forward event through chain
  return no_event;
}


template <typename FunctorT, typename MsgT>
/*static*/ EventType BroadcasterSerializeFunctor<FunctorT,MsgT>::broadcastMsg(
  MsgT* msg, TagType const& tag, ActionType action
) {
  vtAssert(tag == no_tag, "Tagged messages serialized not implemented");
  SerializedMessenger::broadcastSerialMsg<FunctorT,MsgT>(msg);
  // @todo: forward event through chain
  return no_event;
}

}}} /* end namespace vt::serialization::auto_dispatch */

#endif /*INCLUDED_SERIALIZATION_AUTO_DISPATCH_DISPATCH_FUNCTOR_IMPL_H*/
