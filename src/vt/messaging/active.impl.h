/*
//@HEADER
// ************************************************************************
//
//                          active.impl.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_MESSAGING_ACTIVE_IMPL_H
#define INCLUDED_MESSAGING_ACTIVE_IMPL_H

#include "vt/config.h"
#include "vt/messaging/active.h"
#include "vt/termination/term_headers.h"
#include "vt/serialization/auto_dispatch/dispatch.h"
#include "vt/serialization/auto_dispatch/dispatch_handler.h"
#include "vt/serialization/auto_dispatch/dispatch_functor.h"

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
  NodeType const& dest, HandlerType const& han, MsgSharedPtr<MsgT> const& msg
) {
  return sendMsg<MsgT>(dest,han,msg.get());
}

template <typename MsgT>
EventType ActiveMessenger::sendMsg(
  NodeType const& dest, HandlerType const& han, MsgSharedPtr<MsgT> const& msg,
  TagType const& tag
) {
  return sendMsg<MsgT>(dest,han,msg.get(),tag);
}

template <typename MessageT>
EventType ActiveMessenger::sendMsg(
  NodeType const& dest, HandlerType const& han, MessageT* const msg
) {
  return sendMsgSz<MessageT>(dest, han, msg, sizeof(MessageT));
}

template <typename MessageT>
EventType ActiveMessenger::sendMsg(
  NodeType const& dest, HandlerType const& han, MessageT* const msg,
  TagType const& tag
) {
  if (tag != no_tag) {
    envelopeSetTag(msg->env, tag);
  }
  return sendMsgSz<MessageT>(dest, han, msg, sizeof(MessageT));
}

template <typename MessageT>
EventType ActiveMessenger::sendMsgSz(
  NodeType const& dest, HandlerType const& han, MessageT* const msg,
  ByteType const& msg_size
) {
  envelopeSetup(msg->env, dest, han);
  auto base = promoteMsg(msg).template to<BaseMsgType>();
  return sendMsgSized(base, msg_size);
}

template <typename MessageT>
EventType ActiveMessenger::sendMsgAuto(
  NodeType const& dest, HandlerType const& han, MessageT* const msg
) {
  return ActiveSendHandler<MessageT>::sendMsg(dest,msg,han,no_tag);
}

template <typename MessageT>
EventType ActiveMessenger::sendMsgAuto(
  NodeType const& dest, HandlerType const& han, MessageT* const msg,
  TagType const& tag
) {
  return ActiveSendHandler<MessageT>::sendMsg(dest,msg,han,tag);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
EventType ActiveMessenger::broadcastMsgSz(
  MessageT* const msg, ByteType const& msg_size, TagType const& tag
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
  return sendMsgSz(this_node, han, msg, msg_size);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
EventType ActiveMessenger::broadcastMsg(
  MessageT* const msg, TagType const& tag
) {
  return broadcastMsgSz<MessageT,f>(msg,sizeof(MessageT),tag);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
EventType ActiveMessenger::sendMsg(
  NodeType const& dest, MessageT* const msg, TagType const& tag
) {
  return sendMsgSz<MessageT,f>(dest, msg, sizeof(MessageT), tag);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
EventType ActiveMessenger::sendMsgSz(
  NodeType const& dest, MessageT* const msg, ByteType const& msg_size,
  TagType const& tag
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
  return sendMsgSized(base, msg_size);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
EventType ActiveMessenger::sendMsgAuto(
  NodeType const& dest, MessageT* const msg, TagType const& tag
) {
  return ActiveSend<MessageT,f>::sendMsg(dest,msg,tag);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
EventType ActiveMessenger::sendMsgAuto(
  NodeType const& dest, MessageT* const msg
) {
  return ActiveSend<MessageT,f>::sendMsg(dest,msg,no_tag);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
EventType ActiveMessenger::broadcastMsgAuto(
  MessageT* const msg, TagType const& tag
) {
  return ActiveSend<MessageT,f>::broadcastMsg(msg,tag);
}

template <ActiveFnType* f, typename MessageT>
EventType ActiveMessenger::broadcastMsg(
  MessageT* const msg, TagType const& tag
) {
  auto const& han = auto_registry::makeAutoHandler<MessageT,f>(msg);
  auto const& this_node = theContext()->getNode();
  setBroadcastType(msg->env);
  if (tag != no_tag) {
    envelopeSetTag(msg->env, tag);
  }
  return sendMsg(this_node, han, msg);
}

template <ActiveFnType* f, typename MessageT>
EventType ActiveMessenger::sendMsg(
  NodeType const& dest, MessageT* const msg, TagType const& tag
) {
  auto const& han = auto_registry::makeAutoHandler<MessageT,f>(msg);
  envelopeSetup(msg->env, dest, han);
  if (tag != no_tag) {
    envelopeSetTag(msg->env, tag);
  }
  auto base = promoteMsg(msg).template to<BaseMsgType>();
  return sendMsgSized(base, sizeof(MessageT));
}

template <ActiveFnType* f, typename MessageT>
EventType ActiveMessenger::sendMsg(
  NodeType const& dest, MessageT* const msg
) {
  return sendMsg<f,MessageT>(dest,msg,no_tag);
}

template <typename FunctorT, typename MessageT>
EventType ActiveMessenger::broadcastMsg(
  MessageT* const msg, TagType const& tag
) {
  auto const& han =
    auto_registry::makeAutoHandlerFunctor<FunctorT, true, MessageT*>();
  setBroadcastType(msg->env);
  if (tag != no_tag) {
    envelopeSetTag(msg->env, tag);
  }
  return sendMsg(theContext()->getNode(), han, msg);
}

template <typename FunctorT, typename MessageT>
EventType ActiveMessenger::sendMsg(
  NodeType const& dest, MessageT* const msg, TagType const& tag
) {
  auto const& han =
    auto_registry::makeAutoHandlerFunctor<FunctorT, true, MessageT*>();
  envelopeSetup(msg->env, dest, han);
  if (tag != no_tag) {
    envelopeSetTag(msg->env, tag);
  }
  auto base = promoteMsg(msg).template to<BaseMsgType>();
  return sendMsgSized(base, sizeof(MessageT));
}

template <typename FunctorT, typename MessageT>
EventType ActiveMessenger::broadcastMsgAuto(
  MessageT* const msg, TagType const& tag
) {
  return ActiveSendFunctor<FunctorT,MessageT>::broadcastMsg(msg,tag);
}

template <typename FunctorT, typename MessageT>
EventType ActiveMessenger::broadcastMsgAuto(
  MessageT* const msg
) {
  return ActiveSendFunctor<FunctorT,MessageT>::broadcastMsg(msg,no_tag);
}

template <typename FunctorT, typename MessageT>
EventType ActiveMessenger::sendMsgAuto(
  NodeType const& dest, MessageT* const msg, TagType const& tag
) {
  return ActiveSendFunctor<FunctorT,MessageT>::sendMsg(dest,msg,tag);
}

template <typename FunctorT, typename MessageT>
EventType ActiveMessenger::sendMsgAuto(
  NodeType const& dest, MessageT* const msg
) {
  return ActiveSendFunctor<FunctorT,MessageT>::sendMsg(dest,msg,no_tag);
}

template <typename MessageT>
EventType ActiveMessenger::sendMsg(
  NodeType const& dest, HandlerType const& han, MessageT* const msg,
  UserSendFnType send_payload_fn
) {
  namespace ph = std::placeholders;

  // must send first so action payload function runs before the send
  auto f = std::bind(
    &ActiveMessenger::sendData, this, ph::_1, ph::_2, ph::_3
  );
  send_payload_fn(f);

  // setup envelope
  envelopeSetup(msg->env, dest, han);
  auto base = promoteMsg(msg).template to<BaseMsgType>();
  auto const& ret = sendMsgSized(base, sizeof(MessageT));
  return ret;
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
EventType ActiveMessenger::sendMsg(
  NodeType const& dest, MessageT* const msg, UserSendFnType send_payload_fn
) {
  auto const& han = auto_registry::makeAutoHandler<MessageT,f>(msg);
  return sendMsg<MessageT>(dest, han, msg, send_payload_fn);
}

template <typename MsgT>
EventType ActiveMessenger::broadcastMsg(
  HandlerType const& han, MsgSharedPtr<MsgT> const& msg
) {
  return broadcastMsg<MsgT>(han,msg.get());
}

template <typename MsgT>
EventType ActiveMessenger::broadcastMsg(
  HandlerType const& han, MsgSharedPtr<MsgT> const& msg, TagType const& tag
) {
  return broadcastMsg<MsgT>(han,msg.get(),tag);
}

template <typename MessageT>
EventType ActiveMessenger::broadcastMsg(
  HandlerType const& han, MessageT* const msg
) {
  auto const& this_node = theContext()->getNode();
  setBroadcastType(msg->env);
  return sendMsg(this_node, han, msg);
}

template <typename MessageT>
EventType ActiveMessenger::broadcastMsg(
  HandlerType const& han, MessageT* const msg, TagType const& tag
) {
  auto const& this_node = theContext()->getNode();
  setBroadcastType(msg->env);
  if (tag != no_tag) {
    envelopeSetTag(msg->env, tag);
  }
  return sendMsg(this_node, han, msg);
}

template <typename MessageT>
EventType ActiveMessenger::broadcastMsgAuto(
  HandlerType const& han, MessageT* const msg
) {
  return ActiveSendHandler<MessageT>::broadcastMsg(msg,han,no_tag);
}

template <typename MessageT>
EventType ActiveMessenger::broadcastMsgAuto(
  HandlerType const& han, MessageT* const msg, TagType const& tag
) {
  return ActiveSendHandler<MessageT>::broadcastMsg(msg,han,tag);
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
    cur_epoch
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
