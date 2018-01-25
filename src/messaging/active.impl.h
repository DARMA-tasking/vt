
#if !defined INCLUDED_MESSAGING_ACTIVE_IMPL_H
#define INCLUDED_MESSAGING_ACTIVE_IMPL_H

#include "config.h"
#include "messaging/active.h"
#include "termination/term_headers.h"

namespace vt {

template <typename MessageT>
void ActiveMessenger::setTermMessage(MessageT* const msg) {
  setTermType(msg->env);
}

template <typename MessageT>
void ActiveMessenger::setEpochMessage(
  MessageT* const msg, EpochType const& epoch
) {
  envelopeSetEpoch(msg->env, epoch);
}

template <typename MessageT>
void ActiveMessenger::setTagMessage(MessageT* const msg, TagType const& tag) {
  envelopeSetTag(msg->env, tag);
}

template <typename MessageT>
EventType ActiveMessenger::sendMsg(
  NodeType const& dest, HandlerType const& han, MessageT* const msg,
  ActionType next_action
) {
  envelopeSetup(msg->env, dest, han);
  return sendDataDirect(han, msg, sizeof(MessageT), next_action);
}

template <typename MessageT>
EventType ActiveMessenger::sendMsg(
  HandlerType const& han, MessageT* const msg, TagType const& tag,
  ActionType next_action
) {
  auto const& dest = HandlerManagerType::getHandlerNode(han);
  assert(
    dest != uninitialized_destination and
    "Destination must be known in handler"
  );
  envelopeSetup(msg->env, dest, han);
  if (tag != no_tag) {
    envelopeSetTag(msg->env, tag);
  }
  return sendDataDirect(han, msg, sizeof(MessageT), next_action);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
EventType ActiveMessenger::broadcastMsg(
  MessageT* const msg, TagType const& tag, ActionType next_action
) {
  HandlerType const& han = auto_registry::makeAutoHandler<MessageT,f>(msg);
  auto const& this_node = theContext()->getNode();
  setBroadcastType(msg->env);
  if (tag != no_tag) {
    envelopeSetTag(msg->env, tag);
  }
  return sendMsg(this_node, han, msg, next_action);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
EventType ActiveMessenger::broadcastMsg(MessageT* const msg, ActionType act) {
  return broadcastMsg<MessageT,f>(msg,no_tag,act);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
EventType ActiveMessenger::sendMsg(
  NodeType const& dest, MessageT* const msg, TagType const& tag,
  ActionType next_action
) {
  HandlerType const& han = auto_registry::makeAutoHandler<MessageT,f>(msg);
  envelopeSetup(msg->env, dest, han);
  if (tag != no_tag) {
    envelopeSetTag(msg->env, tag);
  }
  return sendDataDirect(han, msg, sizeof(MessageT), next_action);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
EventType ActiveMessenger::sendMsg(
  NodeType const& dest, MessageT* const msg, ActionType act
) {
  return sendMsg<MessageT,f>(dest,msg,no_tag,act);
}

template <ActiveFnType* f, typename MessageT>
EventType ActiveMessenger::broadcastMsg(
  MessageT* const msg, TagType const& tag, ActionType next_action
) {
  HandlerType const& han = auto_registry::makeAutoHandler<MessageT,f>(msg);
  auto const& this_node = theContext()->getNode();
  setBroadcastType(msg->env);
  if (tag != no_tag) {
    envelopeSetTag(msg->env, tag);
  }
  return sendMsg(this_node, han, msg, next_action);
}

template <ActiveFnType* f, typename MessageT>
EventType ActiveMessenger::broadcastMsg(MessageT* const msg, ActionType act) {
  return broadcastMsg<f,MessageT>(msg,no_tag,act);
}

template <ActiveFnType* f, typename MessageT>
EventType ActiveMessenger::sendMsg(
  NodeType const& dest, MessageT* const msg, TagType const& tag,
  ActionType next_action
) {
  HandlerType const& han = auto_registry::makeAutoHandler<MessageT,f>(msg);
  envelopeSetup(msg->env, dest, han);
  if (tag != no_tag) {
    envelopeSetTag(msg->env, tag);
  }
  return sendDataDirect(han, msg, sizeof(MessageT), next_action);
}

template <ActiveFnType* f, typename MessageT>
EventType ActiveMessenger::sendMsg(
  NodeType const& dest, MessageT* const msg, ActionType act
) {
  return sendMsg<f,MessageT>(dest,msg,no_tag,act);
}

template <typename FunctorT, typename MessageT>
EventType ActiveMessenger::broadcastMsg(
  MessageT* const msg, TagType const& tag, ActionType next_action
) {
  HandlerType const& han =
    auto_registry::makeAutoHandlerFunctor<FunctorT, true, MessageT*>();
  setBroadcastType(msg->env);
  if (tag != no_tag) {
    envelopeSetTag(msg->env, tag);
  }
  return sendMsg(theContext()->getNode(), han, msg, next_action);
}

template <typename FunctorT, typename MessageT>
EventType ActiveMessenger::broadcastMsg(MessageT* const msg, ActionType act) {
  return broadcastMsg<FunctorT,MessageT>(msg,no_tag,act);
}

template <typename FunctorT, typename MessageT>
EventType ActiveMessenger::sendMsg(
  NodeType const& dest, MessageT* const msg, TagType const& tag,
  ActionType next_action
) {
  HandlerType const& han =
    auto_registry::makeAutoHandlerFunctor<FunctorT, true, MessageT*>();
  envelopeSetup(msg->env, dest, han);
  if (tag != no_tag) {
    envelopeSetTag(msg->env, tag);
  }
  return sendDataDirect(han, msg, sizeof(MessageT), next_action);
}

template <typename FunctorT, typename MessageT>
EventType ActiveMessenger::sendMsg(
  NodeType const& dest, MessageT* const msg, ActionType act
) {
  return sendMsg<FunctorT,MessageT>(dest,msg,no_tag,act);
}

template <typename MessageT>
EventType ActiveMessenger::sendMsg(
  NodeType const& dest, HandlerType const& han, MessageT* const msg,
  UserSendFnType send_payload_fn, ActionType next_action
) {
  using namespace std::placeholders;

  // must send first so action payload function runs before the send
  auto f = std::bind(&ActiveMessenger::sendData, this, _1, _2, _3, _4);
  send_payload_fn(f);

  // setup envelope
  envelopeSetup(msg->env, dest, han);
  auto const& ret = sendDataDirect(han, msg, sizeof(MessageT), next_action);

  return ret;
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
EventType ActiveMessenger::sendMsg(
  NodeType const& dest, MessageT* const msg, UserSendFnType send_payload_fn,
  ActionType next_action
) {
  HandlerType const& han = auto_registry::makeAutoHandler<MessageT,f>(msg);
  return sendMsg<MessageT>(dest, han, msg, send_payload_fn, next_action);
}

template <typename MessageT>
EventType ActiveMessenger::broadcastMsg(
  HandlerType const& han, MessageT* const msg, ActionType next_action
) {
  auto const& this_node = theContext()->getNode();
  setBroadcastType(msg->env);
  return sendMsg(this_node, han, msg, next_action);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
EventType ActiveMessenger::sendDataCallback(
  NodeType const& dest, MessageT* const msg, ActiveClosureFnType fn
) {
  HandlerType const& this_han = registerNewHandler(fn, no_tag);
  auto cb = static_cast<CallbackMessage*>(msg);
  cb->setCallback(this_han);
  return sendMsg<MessageT,f>(dest, msg, no_tag, nullptr);
}

template <typename MessageT>
void ActiveMessenger::sendDataCallback(
  HandlerType const& han, NodeType const& dest, MessageT* const msg,
  ActiveClosureFnType fn
) {
  HandlerType const& this_han = registerNewHandler(fn, no_tag);
  auto cb = static_cast<CallbackMessage*>(msg);
  cb->setCallback(this_han);
  sendMsg(dest, han, msg);
}

template <typename MessageT>
void ActiveMessenger::sendCallback(MessageT* const msg) {
  auto const& han_callback = getCurrentCallback();
  sendMsg(han_callback, msg);
}


template <typename MessageT, ActiveTypedFnType<MessageT>* f>
void ActiveMessenger::trigger(std::function<void(vt::BaseMessage*)> fn) {
  HandlerType const& han = auto_registry::makeAutoHandler<MessageT,f>(nullptr);
  theRegistry()->saveTrigger(han, /*reinterpret_cast<active_function_t>(*/fn);
}

template <typename MessageT>
EventType ActiveMessenger::basicSendData(
  NodeType const& dest, MessageT* const msg, int const& msg_size,
  bool const& is_shared, bool const& is_term, EpochType const& epoch,
  TagType const& send_tag, EventRecordType* parent_event, ActionType next_action
) {
  auto const& this_node = theContext()->getNode();

  auto const event_id = theEvent()->createMPIEvent(this_node);
  auto& holder = theEvent()->getEventHolder(event_id);
  auto mpi_event = holder.get_event();

  if (is_shared) {
    mpi_event->setManagedMessage(msg);
  }

  if (not is_term) {
    theTerm()->produce(epoch);
  }

  MPI_Isend(
    msg, msg_size, MPI_BYTE, dest, send_tag, theContext()->getComm(),
    mpi_event->getRequest()
  );

  if (parent_event) {
    parent_event->addEventToList(event_id);
  } else {
    holder.attachAction(next_action);
  }

  if (is_shared) {
    messageDeref(msg);
  }

  return event_id;
}

} /* end namespace vt */

#endif /*INCLUDED_MESSAGING_ACTIVE_IMPL_H*/
