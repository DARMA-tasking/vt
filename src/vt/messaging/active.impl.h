/*
//@HEADER
// *****************************************************************************
//
//                                active.impl.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_VT_MESSAGING_ACTIVE_IMPL_H
#define INCLUDED_VT_MESSAGING_ACTIVE_IMPL_H

#include "vt/config.h"
#include "vt/messaging/active.h"
#include "vt/termination/term_headers.h"
#include "vt/messaging/message/message_priority.impl.h"
#include "vt/scheduler/priority.h"
#include "vt/configs/arguments/app_config.h"
#include "vt/serialization/messaging/serialized_messenger.h"
#include "vt/scheduler/thread_action.h"
#include "vt/scheduler/thread_manager.h"
#include "vt/runnable/runnable.h"

namespace vt { namespace messaging {

constexpr NodeType broadcast_dest = uninitialized_destination;

template <typename MsgPtrT>
void ActiveMessenger::markAsTermMessage(MsgPtrT const msg) {
  setTermType(msg->env);
#if vt_check_enabled(priorities)
  envelopeSetPriority(msg->env, sys_min_priority);
#endif
#if vt_check_enabled(trace_enabled)
  envelopeSetTraceRuntimeEnabled(msg->env, theConfig()->traceTerm());
#endif
}

template <typename MsgPtrT>
void ActiveMessenger::markAsLocationMessage(MsgPtrT const msg) {
#if vt_check_enabled(trace_enabled)
  envelopeSetTraceRuntimeEnabled(msg->env, theConfig()->traceLocation());
#endif
}

template <typename MsgPtrT>
void ActiveMessenger::markAsSerialMsgMessage(MsgPtrT const msg) {
#if vt_check_enabled(trace_enabled)
  envelopeSetTraceRuntimeEnabled(msg->env, theConfig()->traceSerialMsg());
#endif
}

template <typename MsgPtrT>
void ActiveMessenger::markAsCollectionMessage(MsgPtrT const msg) {
#if vt_check_enabled(trace_enabled)
  envelopeSetTraceRuntimeEnabled(msg->env, theConfig()->traceCollection());
#endif
}

template <typename MsgT>
void ActiveMessenger::setEpochMessage(MsgT* msg, EpochType epoch) {
  envelopeSetEpoch(msg->env, epoch);
}

template <typename MsgT>
void ActiveMessenger::setTagMessage(MsgT* msg, TagType tag) {
  envelopeSetTag(msg->env, tag);
}

template <typename MsgT>
ActiveMessenger::PendingSendType ActiveMessenger::sendMsgSerializableImpl(
  NodeType dest,
  HandlerType han,
  MsgSharedPtr<MsgT>& msg,
  TagType tag
) {
  // These calls eventually end up back and the non-serialized sendMsgImpl,
  // through use of a wrapped message which does not define serialization.
  // (Although such probably represents an opportunity for additional cleanup.)
  static_assert( // that a message is serializable.
    ::checkpoint::SerializableTraits<MsgT>::is_serializable,
    "Message going through serialization must meet all requirements."
  );

  vtAssert(
    tag == no_tag,
    "Tagged messages serialization not implemented."
  );

  MsgT* rawMsg = msg.get();
  setupEpochMsg(rawMsg);

  // Original message is locked even though it is not the actual message sent.
  // This is for consistency with sending non-serialized messages.
  envelopeSetIsLocked(msg->env, true);

  if (dest == broadcast_dest) {
    return SerializedMessenger::broadcastSerialMsg<MsgT>(
      rawMsg, han, envelopeGetDeliverBcast(rawMsg->env)
    );
  } else {
    return SerializedMessenger::sendSerialMsg<MsgT>(dest, rawMsg, han);
  }
}

template <typename MsgT>
ActiveMessenger::PendingSendType ActiveMessenger::sendMsgCopyableImpl(
  NodeType dest,
  HandlerType han,
  MsgSharedPtr<MsgT>& msg,
  TagType tag
) {
  static_assert(
    vt::messaging::is_byte_copyable_t<MsgT>::value,
    "Message sent without serialization must be byte-copyable."
  );
  static_assert(
    std::is_trivially_destructible<MsgT>::value,
    "Message sent without serialization must be trivially destructible."
  );

  MsgT* rawMsg = msg.get();

  bool is_term = envelopeIsTerm(rawMsg->env);
  const bool is_bcast = dest == broadcast_dest;

  if (!is_term || vt_check_enabled(print_term_msgs)) {
    vt_debug_print(
      verbose, active,
      is_bcast
        ? "broadcastMsg of ptr={}, type={}\n"
        : "sendMsg of ptr={}, type={}\n",
      print_ptr(rawMsg), typeid(MsgT).name()
    );
  }

  if (is_bcast) {
    dest = this_node_;
  }
  if (tag != no_tag) {
    envelopeSetTag(rawMsg->env, tag);
  }
  envelopeSetup(rawMsg->env, dest, han);
  setupEpochMsg(rawMsg);

  envelopeSetIsLocked(rawMsg->env, true);

  auto base = msg.template to<BaseMsgType>();
  return PendingSendType(base);
}

template <typename MsgT>
ActiveMessenger::PendingSendType ActiveMessenger::sendMsg(
  NodeType dest,
  HandlerType han,
  MsgPtrThief<MsgT> msg,
  TagType tag
) {
  MsgSharedPtr<MsgT> msgptr = msg.msg_;
  return sendMsgImpl<MsgT>(dest, han, msgptr, tag);
}

template <typename MsgT>
ActiveMessenger::PendingSendType ActiveMessenger::sendMsgSz(
  NodeType dest,
  HandlerType han,
  MsgPtrThief<MsgT> msg,
  ByteType msg_size,
  TagType tag
) {
  MsgSharedPtr<MsgT> msgptr(msg.msg_, msg_size); // Note: use explicitly provided message size
  return sendMsgImpl<MsgT>(dest, han, msgptr, tag);
}

template <typename MsgT>
ActiveMessenger::PendingSendType ActiveMessenger::sendMsgAuto(
  NodeType dest,
  HandlerType han,
  MsgPtrThief<MsgT> msg,
  TagType tag
) {
  MsgSharedPtr<MsgT> msgptr = msg.msg_;
  return sendMsgImpl<MsgT>(dest, han, msgptr, tag);
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
ActiveMessenger::PendingSendType ActiveMessenger::broadcastMsgSz(
  MsgPtrThief<MsgT> msg,
  ByteType msg_size,
  bool deliver_to_sender,
  TagType tag
) {
  auto const han = auto_registry::makeAutoHandler<MsgT,f>();
  MsgSharedPtr<MsgT> msgptr(msg.msg_);

  setBroadcastType(msgptr->env, deliver_to_sender);

  return sendMsgImpl<MsgT>(
    broadcast_dest, han, msgptr, tag
  );
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
ActiveMessenger::PendingSendType ActiveMessenger::broadcastMsg(
  MsgPtrThief<MsgT> msg,
  bool deliver_to_sender,
  TagType tag
) {
  auto const han = auto_registry::makeAutoHandler<MsgT,f>();
  MsgSharedPtr<MsgT> msgptr = msg.msg_;

  setBroadcastType(msgptr->env, deliver_to_sender);

  return sendMsgImpl<MsgT>(
    broadcast_dest, han, msgptr, tag
  );
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
ActiveMessenger::PendingSendType ActiveMessenger::sendMsg(
  NodeType dest,
  MsgPtrThief<MsgT> msg,
  TagType tag
) {
  auto const han = auto_registry::makeAutoHandler<MsgT,f>();
  MsgSharedPtr<MsgT> msgptr = msg.msg_;
  return sendMsgImpl<MsgT>(dest, han, msgptr, tag);
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
ActiveMessenger::PendingSendType ActiveMessenger::sendMsgSz(
  NodeType dest,
  MsgPtrThief<MsgT> msg,
  ByteType msg_size,
  TagType tag
) {
  auto const han = auto_registry::makeAutoHandler<MsgT,f>();
  MsgSharedPtr<MsgT> msgptr(msg.msg_);
  return sendMsgImpl<MsgT>(dest, han, msgptr, tag);
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
ActiveMessenger::PendingSendType ActiveMessenger::sendMsgAuto(
  NodeType dest,
  MsgPtrThief<MsgT> msg,
  TagType tag
) {
  auto const han = auto_registry::makeAutoHandler<MsgT,f>();
  MsgSharedPtr<MsgT> msgptr = msg.msg_;
  return sendMsgImpl<MsgT>(dest, han, msgptr, tag);
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
ActiveMessenger::PendingSendType ActiveMessenger::broadcastMsgAuto(
  MsgPtrThief<MsgT> msg,
  TagType tag
) {
  auto const han = auto_registry::makeAutoHandler<MsgT,f>();
  MsgSharedPtr<MsgT> msgptr = msg.msg_;
  return sendMsgImpl<MsgT>(
    broadcast_dest, han, msgptr, tag
  );
}

template <ActiveFnType* f, typename MsgT>
ActiveMessenger::PendingSendType ActiveMessenger::broadcastMsg(
  MsgPtrThief<MsgT> msg,
  bool deliver_to_sender,
  TagType tag
) {
  auto const han = auto_registry::makeAutoHandler<MsgT,f>();
  MsgSharedPtr<MsgT> msgptr = msg.msg_;
  setBroadcastType(msgptr->env, deliver_to_sender);
  return sendMsgImpl<MsgT>(
    broadcast_dest, han, msgptr, tag
  );
}

template <ActiveFnType* f, typename MsgT>
ActiveMessenger::PendingSendType ActiveMessenger::sendMsg(
  NodeType dest,
  MsgPtrThief<MsgT> msg,
  TagType tag
) {
  auto const han = auto_registry::makeAutoHandler<MsgT,f>();
  MsgSharedPtr<MsgT> msgptr = msg.msg_;
  return sendMsgImpl<MsgT>(dest, han, msgptr, tag);
}

template <typename FunctorT, typename MsgT>
ActiveMessenger::PendingSendType ActiveMessenger::broadcastMsg(
  MsgPtrThief<MsgT> msg,
  bool deliver_to_sender,
  TagType tag
) {
  auto const han = auto_registry::makeAutoHandlerFunctor<FunctorT, MsgT, true>();
  MsgSharedPtr<MsgT> msgptr = msg.msg_;
  setBroadcastType(msgptr->env, deliver_to_sender);
  return sendMsgImpl<MsgT>(
    broadcast_dest, han, msgptr, tag
  );
}

template <typename FunctorT>
ActiveMessenger::PendingSendType ActiveMessenger::broadcastMsg(
  MsgPtrThief<typename util::FunctorExtractor<FunctorT>::MessageType> msg,
  bool deliver_to_sender,
  TagType tag
) {
  using MsgT = typename util::FunctorExtractor<FunctorT>::MessageType;
  return broadcastMsg<FunctorT, MsgT>(msg, deliver_to_sender, tag);
}

template <typename FunctorT, typename MsgT>
ActiveMessenger::PendingSendType ActiveMessenger::sendMsg(
  NodeType dest,
  MsgPtrThief<MsgT> msg,
  TagType tag
) {
  auto const han = auto_registry::makeAutoHandlerFunctor<FunctorT, MsgT, true>();
  MsgSharedPtr<MsgT> msgptr = msg.msg_;
  return sendMsgImpl<MsgT>(dest, han, msgptr, tag);
}

template <typename FunctorT>
ActiveMessenger::PendingSendType ActiveMessenger::sendMsg(
  NodeType dest,
  MsgPtrThief<typename util::FunctorExtractor<FunctorT>::MessageType> msg,
  TagType tag
) {
  using MsgT = typename util::FunctorExtractor<FunctorT>::MessageType;
  return sendMsg<FunctorT, MsgT>(dest, msg, tag);
}

template <typename FunctorT, typename MsgT>
ActiveMessenger::PendingSendType ActiveMessenger::broadcastMsgAuto(
  MsgPtrThief<MsgT> msg,
  TagType tag
) {
  auto const han = auto_registry::makeAutoHandlerFunctor<FunctorT, MsgT, true>();
  MsgSharedPtr<MsgT> msgptr = msg.msg_;
  return sendMsgImpl<MsgT>(
    broadcast_dest, han, msgptr, tag
  );
}

template <typename FunctorT, typename MsgT>
ActiveMessenger::PendingSendType ActiveMessenger::sendMsgAuto(
  NodeType dest,
  MsgPtrThief<MsgT> msg,
  TagType tag
) {
  auto const han = auto_registry::makeAutoHandlerFunctor<FunctorT, MsgT, true>();
  MsgSharedPtr<MsgT> msgptr = msg.msg_;
  return sendMsgImpl<MsgT>(dest, han, msgptr, tag);
}

template <typename MsgT>
ActiveMessenger::PendingSendType ActiveMessenger::sendMsg(
  NodeType dest,
  HandlerType han,
  MsgPtrThief<MsgT> msg,
  UserSendFnType send_payload_fn
) {
  namespace ph = std::placeholders;

  // must send first so action payload function runs before the send
  auto f = std::bind(
    &ActiveMessenger::sendData, this, ph::_1, ph::_2, ph::_3
  );
  send_payload_fn(f);

  MsgSharedPtr<MsgT> msgptr = msg.msg_;
  auto tag = no_tag;
  return sendMsgImpl<MsgT>(dest, han, msgptr, tag);
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
ActiveMessenger::PendingSendType ActiveMessenger::sendMsg(
  NodeType dest,
  MsgPtrThief<MsgT> msg,
  UserSendFnType send_payload_fn
) {
  auto const han = auto_registry::makeAutoHandler<MsgT,f>();
  return sendMsg<MsgT>(dest, han, msg, send_payload_fn);
}

template <typename MsgT>
ActiveMessenger::PendingSendType ActiveMessenger::broadcastMsg(
  HandlerType han,
  MsgPtrThief<MsgT> msg,
  bool deliver_to_sender,
  TagType tag
) {
  MsgSharedPtr<MsgT> msgptr = msg.msg_;
  setBroadcastType(msgptr->env, deliver_to_sender);
  return sendMsgImpl<MsgT>(
    broadcast_dest, han, msgptr, tag
  );
}

template <typename MsgT>
ActiveMessenger::PendingSendType ActiveMessenger::broadcastMsgAuto(
  HandlerType han,
  MsgPtrThief<MsgT> msg,
  TagType tag
) {
  MsgSharedPtr<MsgT> msgptr = msg.msg_;
  return sendMsgImpl<MsgT>(
    broadcast_dest, han, msgptr, tag
  );
}

inline void ActiveMessenger::pushEpoch(EpochType const& epoch) {
  return theTerm()->pushEpoch(epoch);
}

inline EpochType ActiveMessenger::popEpoch(EpochType const& epoch) {
  return theTerm()->popEpoch(epoch);
}

inline EpochType ActiveMessenger::getEpoch() const {
  return theTerm()->getEpoch();
}

template <typename MsgT>
inline EpochType ActiveMessenger::getEpochContextMsg(MsgT* msg) {
  auto const is_epoch = envelopeIsEpochType(msg->env);
  auto const any_epoch = term::any_epoch_sentinel;

  if (is_epoch) {
    auto const msg_epoch = envelopeGetEpoch(msg->env);

    // Prefer the epoch on the msg before the current handler epoch
    if (msg_epoch != no_epoch and msg_epoch != any_epoch) {
      // It has a valid (possibly global) epoch, use it
      return msg_epoch;
    } else {
      // Otherwise, use the active messenger's current epoch on the stack
      auto const ctx_epoch = getEpoch();
      vtAssertInfo(
        ctx_epoch != no_epoch, "Must have a valid epoch here",
        ctx_epoch, msg_epoch, no_epoch, any_epoch
      );
      return ctx_epoch;
    }
  } else {
    return any_epoch;
  }
}

template <typename MsgT>
inline EpochType ActiveMessenger::getEpochContextMsg(
  MsgSharedPtr<MsgT> const& msg
) {
  return getEpochContextMsg(msg.get());
}

template <typename MsgT>
inline EpochType ActiveMessenger::setupEpochMsg(MsgT* msg) {
  auto const is_epoch = envelopeIsEpochType(msg->env);
  if (is_epoch) {
    auto const epoch = getEpochContextMsg(msg);
    bool const msg_no_epoch = envelopeGetEpoch(msg->env) == no_epoch;
    if (msg_no_epoch and epoch != term::any_epoch_sentinel) {
      setEpochMessage(msg, epoch);
    }
    return epoch;
  } else {
    return term::any_epoch_sentinel;
  }
}

template <typename MsgT>
inline EpochType ActiveMessenger::setupEpochMsg(MsgSharedPtr<MsgT> const& msg) {
  return setupEpochMsg(msg.get());
}

}} //end namespace vt::messaging

#endif /*INCLUDED_VT_MESSAGING_ACTIVE_IMPL_H*/
