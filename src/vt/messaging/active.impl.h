
#if !defined INCLUDED_MESSAGING_ACTIVE_IMPL_H
#define INCLUDED_MESSAGING_ACTIVE_IMPL_H

#include "config.h"
#include "messaging/active.h"
#include "termination/term_headers.h"
#include "serialization/auto_dispatch/dispatch.h"
#include "serialization/auto_dispatch/dispatch_handler.h"
#include "serialization/auto_dispatch/dispatch_functor.h"

namespace vt { namespace messaging {

template <typename MsgPtrT>
void ActiveMessenger::setTermMessage(MsgPtrT const msg) {
  setTermType(msg->env);
}

template <typename MsgPtrT>
void ActiveMessenger::setEpochMessage(MsgPtrT msg, EpochType const& epoch) {
  envelopeSetEpoch(msg->env, epoch);
}

template <typename MsgPtrT>
void ActiveMessenger::setTagMessage(MsgPtrT msg, TagType const& tag) {
  envelopeSetTag(msg->env, tag);
}

template <typename MsgT>
EventType ActiveMessenger::sendMsg(
  NodeType const& dest, HandlerType const& han, MsgSharedPtr<MsgT> const& msg,
  ActionType next_action
) {
  return sendMsg<MsgT>(dest,han,msg.get(),next_action);
}

template <typename MsgT>
EventType ActiveMessenger::sendMsg(
  NodeType const& dest, HandlerType const& han, MsgSharedPtr<MsgT> const& msg,
  TagType const& tag, ActionType next_action
) {
  return sendMsg<MsgT>(dest,han,msg.get(),tag,next_action);
}

template <typename MsgT>
EventType ActiveMessenger::sendMsg(
  HandlerType const& han, MsgSharedPtr<MsgT> const& msg, TagType const& tag,
  ActionType next_action
) {
  return sendMsg<MsgT>(han,msg.get(),tag,next_action);
}

template <typename MessageT>
EventType ActiveMessenger::sendMsg(
  NodeType const& dest, HandlerType const& han, MessageT* const msg,
  ActionType next_action
) {
  return sendMsgSz<MessageT>(dest, han, msg, next_action, sizeof(MessageT));
}

template <typename MessageT>
EventType ActiveMessenger::sendMsg(
  NodeType const& dest, HandlerType const& han, MessageT* const msg,
  TagType const& tag, ActionType next_action
) {
  if (tag != no_tag) {
    envelopeSetTag(msg->env, tag);
  }
  return sendMsgSz<MessageT>(dest, han, msg, next_action, sizeof(MessageT));
}

template <typename MessageT>
EventType ActiveMessenger::sendMsgSz(
  NodeType const& dest, HandlerType const& han, MessageT* const msg,
  ActionType next_action, ByteType const& msg_size
) {
  envelopeSetup(msg->env, dest, han);
  auto base = promoteMsg(msg).template to<BaseMsgType>();
  return sendMsgSized(han, base, msg_size, next_action);
}

template <typename MessageT>
EventType ActiveMessenger::sendMsgHan(
  HandlerType const& han, MessageT* const msg, TagType const& tag,
  ActionType next_action
) {
  return sendMsg(han,msg,tag,next_action);
}

template <typename MessageT>
EventType ActiveMessenger::sendMsg(
  HandlerType const& han, MessageT* const msg, TagType const& tag,
  ActionType next_action
) {
  auto const& dest = HandlerManagerType::getHandlerNode(han);
  vtAssert(
    dest != uninitialized_destination,
    "Destination must be known in handler"
  );
  envelopeSetup(msg->env, dest, han);
  if (tag != no_tag) {
    envelopeSetTag(msg->env, tag);
  }
  auto const& is_term = envelopeIsTerm(msg->env);
  if (!is_term || backend_check_enabled(print_term_msgs)) {
    debug_print(
      pool, node,
      "sendMsg of indirect han={}, dest={}, tag={}\n", han, dest, tag
    );
  }
  debug_print(
    active, node,
    "sendMsg: (handler) han={}, msg={}, tag={}\n",
    han, print_ptr(msg), tag
  );
  auto base = promoteMsg(msg).template to<BaseMsgType>();
  return sendMsgSized(han, base, sizeof(MessageT), next_action);
}

template <typename MessageT>
EventType ActiveMessenger::sendMsgAuto(
  NodeType const& dest, HandlerType const& han, MessageT* const msg,
  ActionType act
) {
  return ActiveSendHandler<MessageT>::sendMsg(dest,msg,han,no_tag,act);
}

template <typename MessageT>
EventType ActiveMessenger::sendMsgAuto(
  NodeType const& dest, HandlerType const& han, MessageT* const msg,
  TagType const& tag, ActionType act
) {
  return ActiveSendHandler<MessageT>::sendMsg(dest,msg,han,tag,act);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
EventType ActiveMessenger::broadcastMsgSz(
  MessageT* const msg, ByteType const& msg_size, TagType const& tag,
  ActionType next_action
) {
  auto const& is_term = envelopeIsTerm(msg->env);
  if (!is_term || backend_check_enabled(print_term_msgs)) {
    debug_print(
      pool, node,
      "broadcastMsg of ptr={}, type={}\n",
      print_ptr(msg), typeid(MessageT).name()
    );
  }
  auto const& han = auto_registry::makeAutoHandler<MessageT,f>(msg);
  auto const& this_node = theContext()->getNode();
  setBroadcastType(msg->env);
  if (tag != no_tag) {
    envelopeSetTag(msg->env, tag);
  }
  return sendMsgSz(this_node, han, msg, next_action, msg_size);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
EventType ActiveMessenger::broadcastMsg(
  MessageT* const msg, TagType const& tag, ActionType next_action
) {
  return broadcastMsgSz<MessageT,f>(msg,sizeof(MessageT),no_tag,next_action);
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
  return sendMsgSz<MessageT,f>(dest, msg, sizeof(MessageT), tag, next_action);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
EventType ActiveMessenger::sendMsgSz(
  NodeType const& dest, MessageT* const msg, ByteType const& msg_size,
  TagType const& tag, ActionType next_action
) {
  auto const& is_term = envelopeIsTerm(msg->env);
  if (!is_term || backend_check_enabled(print_term_msgs)) {
    debug_print(
      active, node,
      "sendMsg of ptr={}, type={}\n",
      print_ptr(msg), typeid(MessageT).name()
    );
  }
  auto const& han = auto_registry::makeAutoHandler<MessageT,f>(msg);
  envelopeSetup(msg->env, dest, han);
  if (tag != no_tag) {
    envelopeSetTag(msg->env, tag);
  }
  auto base = promoteMsg(msg).template to<BaseMsgType>();;
  return sendMsgSized(han, base, msg_size, next_action);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
EventType ActiveMessenger::sendMsg(
  NodeType const& dest, MessageT* const msg, ActionType act
) {
  return sendMsg<MessageT,f>(dest,msg,no_tag,act);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
EventType ActiveMessenger::sendMsgAuto(
  NodeType const& dest, MessageT* const msg, TagType const& tag,
  ActionType act
) {
  return ActiveSend<MessageT,f>::sendMsg(dest,msg,tag,act);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
EventType ActiveMessenger::sendMsgAuto(
  NodeType const& dest, MessageT* const msg, ActionType act
) {
  return ActiveSend<MessageT,f>::sendMsg(dest,msg,no_tag,act);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
EventType ActiveMessenger::broadcastMsgAuto(
  MessageT* const msg, TagType const& tag, ActionType act
) {
  return ActiveSend<MessageT,f>::broadcastMsg(msg,tag,act);
}

template <ActiveFnType* f, typename MessageT>
EventType ActiveMessenger::broadcastMsg(
  MessageT* const msg, TagType const& tag, ActionType next_action
) {
  auto const& han = auto_registry::makeAutoHandler<MessageT,f>(msg);
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
  auto const& han = auto_registry::makeAutoHandler<MessageT,f>(msg);
  envelopeSetup(msg->env, dest, han);
  if (tag != no_tag) {
    envelopeSetTag(msg->env, tag);
  }
  auto base = promoteMsg(msg).template to<BaseMsgType>();
  return sendMsgSized(han, base, sizeof(MessageT), next_action);
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
  auto const& han =
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
  auto const& han =
    auto_registry::makeAutoHandlerFunctor<FunctorT, true, MessageT*>();
  envelopeSetup(msg->env, dest, han);
  if (tag != no_tag) {
    envelopeSetTag(msg->env, tag);
  }
  auto base = promoteMsg(msg).template to<BaseMsgType>();
  return sendMsgSized(han, base, sizeof(MessageT), next_action);
}

template <typename FunctorT, typename MessageT>
EventType ActiveMessenger::sendMsg(
  NodeType const& dest, MessageT* const msg, ActionType act
) {
  return sendMsg<FunctorT,MessageT>(dest,msg,no_tag,act);
}

template <typename FunctorT, typename MessageT>
EventType ActiveMessenger::broadcastMsgAuto(
  MessageT* const msg, TagType const& tag, ActionType act
) {
  return ActiveSendFunctor<FunctorT,MessageT>::broadcastMsg(msg,tag,act);
}

template <typename FunctorT, typename MessageT>
EventType ActiveMessenger::broadcastMsgAuto(
  MessageT* const msg, ActionType act
) {
  return ActiveSendFunctor<FunctorT,MessageT>::broadcastMsg(msg,no_tag,act);
}

template <typename FunctorT, typename MessageT>
EventType ActiveMessenger::sendMsgAuto(
  NodeType const& dest, MessageT* const msg, TagType const& tag,
  ActionType act
) {
  return ActiveSendFunctor<FunctorT,MessageT>::sendMsg(dest,msg,tag,act);
}

template <typename FunctorT, typename MessageT>
EventType ActiveMessenger::sendMsgAuto(
  NodeType const& dest, MessageT* const msg, ActionType act
) {
  return ActiveSendFunctor<FunctorT,MessageT>::sendMsg(dest,msg,no_tag,act);
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
  auto base = promoteMsg(msg).template to<BaseMsgType>();
  auto const& ret = sendMsgSized(han, base, sizeof(MessageT), next_action);
  return ret;
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
EventType ActiveMessenger::sendMsg(
  NodeType const& dest, MessageT* const msg, UserSendFnType send_payload_fn,
  ActionType next_action
) {
  auto const& han = auto_registry::makeAutoHandler<MessageT,f>(msg);
  return sendMsg<MessageT>(dest, han, msg, send_payload_fn, next_action);
}

template <typename MsgT>
EventType ActiveMessenger::broadcastMsg(
  HandlerType const& han, MsgSharedPtr<MsgT> const& msg, ActionType act
) {
  return broadcastMsg<MsgT>(han,msg.get(),act);
}

template <typename MsgT>
EventType ActiveMessenger::broadcastMsg(
  HandlerType const& han, MsgSharedPtr<MsgT> const& msg, TagType const& tag,
  ActionType act
) {
  return broadcastMsg<MsgT>(han,msg.get(),tag,act);
}

template <typename MessageT>
EventType ActiveMessenger::broadcastMsg(
  HandlerType const& han, MessageT* const msg, ActionType next_action
) {
  auto const& this_node = theContext()->getNode();
  setBroadcastType(msg->env);
  return sendMsg(this_node, han, msg, next_action);
}

template <typename MessageT>
EventType ActiveMessenger::broadcastMsg(
  HandlerType const& han, MessageT* const msg, TagType const& tag,
  ActionType next_action
) {
  auto const& this_node = theContext()->getNode();
  setBroadcastType(msg->env);
  if (tag != no_tag) {
    envelopeSetTag(msg->env, tag);
  }
  return sendMsg(this_node, han, msg, next_action);
}

template <typename MessageT>
EventType ActiveMessenger::broadcastMsgAuto(
  HandlerType const& han, MessageT* const msg, ActionType act
) {
  return ActiveSendHandler<MessageT>::broadcastMsg(msg,han,no_tag,act);
}

template <typename MessageT>
EventType ActiveMessenger::broadcastMsgAuto(
  HandlerType const& han, MessageT* const msg, TagType const& tag,
  ActionType act
) {
  return ActiveSendHandler<MessageT>::broadcastMsg(msg,han,tag,act);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
EventType ActiveMessenger::sendDataCallback(
  NodeType const& dest, MessageT* const msg, ActiveClosureFnType fn
) {
  auto const& this_han = registerNewHandler(fn, no_tag);
  auto cb = static_cast<CallbackMessage*>(msg);
  cb->setCallback(this_han);
  debug_print(
    active, node,
    "sendDataCallback: msg={}, this_han={}, dest={}\n",
    print_ptr(msg), this_han, dest
  );
  return sendMsg<MessageT,f>(dest, msg, no_tag, nullptr);
}

template <typename MessageT>
void ActiveMessenger::sendDataCallback(
  HandlerType const& han, NodeType const& dest, MessageT* const msg,
  ActiveClosureFnType fn
) {
  auto const& this_han = registerNewHandler(fn, no_tag);
  auto cb = static_cast<CallbackMessage*>(msg);
  cb->setCallback(this_han);
  debug_print(
    active, node,
    "sendDataCallback: msg={}, han={}, dest={}\n",
    print_ptr(msg), han, dest
  );
  sendMsg(dest, han, msg);
}

template <typename MessageT>
void ActiveMessenger::sendCallback(MessageT* const msg) {
  auto const& han_callback = getCurrentCallback();
  debug_print(
    active, node,
    "sendCallback: msg={}, han_callback={}\n",
    print_ptr(msg), han_callback
  );
  sendMsgHan<MessageT>(han_callback, msg);
}


template <typename MessageT, ActiveTypedFnType<MessageT>* f>
void ActiveMessenger::trigger(std::function<void(vt::BaseMessage*)> fn) {
  auto const& han = auto_registry::makeAutoHandler<MessageT,f>(nullptr);
  theRegistry()->saveTrigger(han, /*reinterpret_cast<active_function_t>(*/fn);
}

inline ActiveMessenger::EpochStackSizeType
ActiveMessenger::epochPreludeHandler(EpochType const& cur_epoch) {
  debug_print(
    active, node,
    "epochPreludeHandler: top={:x}, cur_epoch={:x}, size={}\n",
    epoch_stack_.size() > 0 ? epoch_stack_.top(): no_epoch, cur_epoch,
    epoch_stack_.size()
  );

  return epoch_stack_.push(cur_epoch),epoch_stack_.size();
}

inline void ActiveMessenger::epochEpilogHandler(
  EpochType const& cur_epoch, EpochStackSizeType const& ep_stack_size
) {
  EpochStackSizeType cur_stack_size = epoch_stack_.size();

  debug_print(
    active, node,
    "epochEpilogHandler: top={:x}, size={}\n",
    epoch_stack_.size() > 0 ? epoch_stack_.top(): no_epoch,
    cur_stack_size
  );

  vtAssertNot(
    ep_stack_size < cur_stack_size,
    "Epoch stack popped below preceding push size in handler"
  );
  vtWarnInfo(
    ep_stack_size == cur_stack_size, "Stack must be same size",
    ep_stack_size, cur_stack_size,
    cur_stack_size > 0 ? epoch_stack_.top() : no_epoch,
    current_handler_context_, current_epoch_context_, current_node_context_,
    current_callback_context_, cur_epoch
  );
  vtAssertNotExpr(cur_stack_size == 0);
  while (cur_stack_size > ep_stack_size) {
    cur_stack_size = (epoch_stack_.pop(), epoch_stack_.size());
  }
  vtAssertExpr(epoch_stack_.top() == cur_epoch);
  vtAssertExpr(cur_stack_size == ep_stack_size);
  epoch_stack_.pop();
}

inline void ActiveMessenger::setGlobalEpoch(EpochType const& epoch) {
  /*
   * setGlobalEpoch() is a shortcut for both pushing and popping epochs on the
   * stack depending on the value of the `epoch' passed as an argument.
   */
  if (epoch == no_epoch) {
    vtAssertInfo(
      epoch_stack_.size() > 0, "Setting no global epoch requires non-zero size",
      epoch_stack_.size()
    );
    if (epoch_stack_.size() > 0) {
      epoch_stack_.pop();
    }
  } else {
    epoch_stack_.push(epoch);
  }
}

inline EpochType ActiveMessenger::getGlobalEpoch() const {
  vtAssertInfo(
    epoch_stack_.size() > 0, "Epoch stack size must be greater than zero",
    epoch_stack_.size()
  );
  return epoch_stack_.size() ? epoch_stack_.top() : term::any_epoch_sentinel;
}

inline void ActiveMessenger::pushEpoch(EpochType const& epoch) {
  /*
   * pushEpoch(epoch) pushes any epoch onto the local stack iff epoch !=
   * no_epoch; the epoch stack includes all locally pushed epochs and the
   * current contexts pushed, transitively causally related active message
   * handlers.
   */
  vtAssertInfo(
    epoch != no_epoch, "Do not push no_epoch onto the epoch stack",
    epoch, no_epoch, epoch_stack_.size(),
    epoch_stack_.size() > 0 ? epoch_stack_.top() : no_epoch
  );
  if (epoch != no_epoch) {
    epoch_stack_.push(epoch);
  }
}

inline EpochType ActiveMessenger::popEpoch(EpochType const& epoch) {
  /*
   * popEpoch(epoch) shall remove the top entry from epoch_size_, iif the size
   * is non-zero and the `epoch' passed, if `epoch != no_epoch', is equal to the
   * top of the `epoch_stack_.top()'; else, it shall remove any entry from the
   * top of the stack.
   */
  auto const& non_zero = epoch_stack_.size() > 0;
  if (epoch == no_epoch) {
    return non_zero ? epoch_stack_.pop(),epoch_stack_.top() : no_epoch;
  } else {
    return non_zero && epoch == epoch_stack_.top() ?
      epoch_stack_.pop(),epoch :
      no_epoch;
  }
}

inline EpochType ActiveMessenger::getEpoch() const {
  return getGlobalEpoch();
}

}} //end namespace vt::messaging

#endif /*INCLUDED_MESSAGING_ACTIVE_IMPL_H*/
