/*
//@HEADER
// *****************************************************************************
//
//                         serialized_messenger.impl.h
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

#if !defined INCLUDED_VT_SERIALIZATION_MESSAGING_SERIALIZED_MESSENGER_IMPL_H
#define INCLUDED_VT_SERIALIZATION_MESSAGING_SERIALIZED_MESSENGER_IMPL_H

#include "vt/config.h"
#include "vt/messaging/message.h"
#include "vt/messaging/active.h"
#include "vt/registry/auto/auto_registry_interface.h"
#include "vt/registry/auto/vc/auto_registry_vc.h"
#include "vt/serialization/messaging/serialized_data_msg.h"
#include "vt/serialization/messaging/serialized_messenger.h"
#include "vt/messaging/envelope/envelope_set.h" // envelopeSetRef
#include "vt/runnable/make_runnable.h"

#include <tuple>
#include <type_traits>
#include <cstdlib>
#include <cassert>

namespace vt { namespace serialization {

template <typename MsgT>
static MsgPtr<MsgT> deserializeFullMessage(SerialByteType* source) {
  MsgT* msg = detail::makeMessageImpl<MsgT>();
  checkpoint::deserializeInPlace<MsgT>(source, msg);

  envelopeInitRecv(msg->env);
  return MsgPtr<MsgT>(msg);
}

template <typename UserMsgT>
/*static*/ void SerializedMessenger::serialMsgHandlerBcast(
  SerialWrapperMsgType<UserMsgT>* sys_msg
) {
  auto const handler = sys_msg->handler;
  auto const ptr_size = sys_msg->ptr_size;

  vt_debug_print(
    normal, serial_msg,
    "serialMsgHandlerBcast: group_={:x}, handler={}, ptr_size={}\n",
    envelopeGetGroup(sys_msg->env), handler, ptr_size
  );

  auto ptr_offset = reinterpret_cast<char*>(sys_msg)
    + sizeof(SerialWrapperMsgType<UserMsgT>);
  auto msg_data = ptr_offset;
  auto user_msg = deserializeFullMessage<UserMsgT>(msg_data);

  runnable::makeRunnable(user_msg, true, handler, sys_msg->from_node)
    .withTDEpochFromMsg()
    .enqueue();
}

template <typename UserMsgT>
/*static*/ void SerializedMessenger::serialMsgHandler(
  SerialWrapperMsgType<UserMsgT>* sys_msg
) {
  auto const handler = sys_msg->handler;
  auto const recv_tag = sys_msg->data_recv_tag;
  auto const nchunks = sys_msg->nchunks;
  auto const len = sys_msg->ptr_size;
  auto const epoch = envelopeGetEpoch(sys_msg->env);

  vt_debug_print(
    normal, serial_msg,
    "serialMsgHandler: non-eager, recvDataMsg: msg={}, handler={}, "
    "recv_tag={}, epoch={}\n",
    print_ptr(sys_msg), handler, recv_tag, epoch
  );

  bool const is_valid_epoch = epoch != no_epoch;

  if (is_valid_epoch) {
    theTerm()->produce(epoch);
  }

  auto node = sys_msg->from_node;
  theMsg()->recvDataDirect(
    nchunks, recv_tag, sys_msg->from_node, len,
    [handler, recv_tag, node, epoch, is_valid_epoch]
    (RDMA_GetType ptr, ActionType action) {
      // be careful here not to use "sys_msg", it is no longer valid
      auto msg_data = reinterpret_cast<SerialByteType*>(std::get<0>(ptr));
      auto msg = deserializeFullMessage<UserMsgT>(msg_data);

      vt_debug_print(
        normal, serial_msg,
        "serialMsgHandler: recvDataMsg finished: handler={}, recv_tag={},"
        "epoch={}\n",
        handler, recv_tag, envelopeGetEpoch(msg->env)
      );

      runnable::makeRunnable(msg, true, handler, node)
        .withTDEpoch(epoch, not is_valid_epoch)
        .withContinuation(action)
        .enqueue();

      if (is_valid_epoch) {
        theTerm()->consume(epoch);
      }
    }
  );
}

template <typename UserMsgT, typename BaseEagerMsgT>
/*static*/ void SerializedMessenger::payloadMsgHandler(
  SerialEagerPayloadMsg<UserMsgT, BaseEagerMsgT>* sys_msg
) {
  auto const handler = sys_msg->handler;

  vt_debug_print(
    normal, serial_msg,
    "payloadMsgHandler: handler={}, bytes={}, epoch={}\n",
    handler, sys_msg->bytes, envelopeGetEpoch(sys_msg->env)
  );

  auto msg_data = sys_msg->payload.data();
  auto user_msg = deserializeFullMessage<UserMsgT>(msg_data);

  // Keep bcast related data in user_msg since it's sometimes
  // needed in the handler
  if (envelopeIsBcast(sys_msg->env)) {
    envelopeCopyBcastData(user_msg->env, sys_msg->env);
  }

  vt_debug_print(
    normal, serial_msg,
    "payloadMsgHandler: group={:x}, msg={}, handler={}, bytes={}, "
    "user ref={}, sys ref={}, user_msg={}, epoch={}\n",
    envelopeGetGroup(sys_msg->env), print_ptr(sys_msg), handler, sys_msg->bytes,
    envelopeGetRef(user_msg->env), envelopeGetRef(sys_msg->env),
    print_ptr(user_msg.get()), envelopeGetEpoch(sys_msg->env)
  );

  runnable::makeRunnable(user_msg, true, handler, sys_msg->from_node)
    .withTDEpochFromMsg()
    .enqueue();
}

template <typename MsgT, typename BaseT>
/*static*/ messaging::PendingSend SerializedMessenger::sendSerialMsg(
  NodeType dest, MsgT* msg, HandlerType handler,
  ActionEagerSend<MsgT, BaseT> eager_sender
) {
  auto eager_default_send =
    [=](MsgSharedPtr<SerializedEagerMsg<MsgT,BaseT>> m) -> messaging::PendingSend {
    using MsgType = SerialEagerPayloadMsg<MsgT,BaseT>;
    theMsg()->markAsSerialMsgMessage(m);
    return theMsg()->sendMsg<MsgType,payloadMsgHandler>(dest, m);
  };
  auto eager = eager_sender ? eager_sender : eager_default_send;
  return sendSerialMsgSendImpl<MsgT,BaseT>(
    msg, handler, eager,
    [=](ActionNodeSendType action) -> messaging::PendingSend {
      return action(dest);
    }
  );
}

template <typename MsgT, typename BaseT>
/*static*/ messaging::PendingSend
 SerializedMessenger::broadcastSerialMsg(
  MsgT* msg_ptr, HandlerType han, bool deliver_to_sender
) {
  using PayloadMsg = SerialEagerPayloadMsg<MsgT, BaseT>;

  auto msg = promoteMsg(msg_ptr);

  MsgSharedPtr<SerialEagerPayloadMsg<MsgT,BaseT>> payload_msg = nullptr;
  MsgSharedPtr<SerialWrapperMsgType<MsgT>> sys_msg = nullptr;
  SizeType ptr_size = 0;
  auto sys_size = sizeof(typename decltype(sys_msg)::MsgType);

  envelopeSetIsLocked(msg->env, true); // implies locked on deserialize
  envelopeSetHasBeenSerialized(msg->env, false);

  auto serialized_msg = checkpoint::serialize(
    *msg.get(), [&](SizeType size) -> SerialByteType* {
      ptr_size = size;
      if (size >= serialized_msg_eager_size) {
        sys_msg = makeMessageSz<SerialWrapperMsgType<MsgT>>(ptr_size);
        return reinterpret_cast<char*>(sys_msg.get()) + sys_size;
      } else {
        payload_msg = makeMessage<PayloadMsg>(
          static_cast<NumBytesType>(size)
        );
        return payload_msg->payload.data();
      }
    }
  );

  vtAssertInfo(
    envelopeHasBeenSerialized(msg->env),
    "A message was serialized that did not correctly serialize parents."
    " All serialized messages are required to call serialize on the"
    " message's parent type. See 'msg_serialize_parent'.",
    typeid(MsgT).name()
  );

  if (ptr_size < serialized_msg_eager_size) {
    vtAssert(
      ptr_size < serialized_msg_eager_size,
      "Must be smaller for eager protocol"
    );

    // wrap metadata
    payload_msg->handler = han;
    payload_msg->from_node = theContext()->getNode();
    // setup envelope
    envelopeInitCopy(payload_msg->env, msg->env);

    vt_debug_print(
      normal, serial_msg,
      "broadcastSerialMsg (eager): han={}, size={}, "
      "serialized_msg_eager_size={}, group={:x}\n",
      han, ptr_size, serialized_msg_eager_size, envelopeGetGroup(msg->env)
    );

    theMsg()->markAsSerialMsgMessage(payload_msg);
    return theMsg()->broadcastMsg<PayloadMsg,payloadMsgHandler>(payload_msg, deliver_to_sender);
  } else {
    auto const& total_size = ptr_size + sys_size;

    auto traceable_han = han;

#   if vt_check_enabled(trace_enabled)
      // Since we aren't sending the message (just packing it into a buffer, we
      // need to transfer whether the handler should be traced on that message
      auto_registry::HandlerManagerType::setHandlerTrace(
        traceable_han, envelopeGetTraceRuntimeEnabled(msg->env)
      );
#   endif

    // wrap metadata
    sys_msg->handler = traceable_han;
    sys_msg->from_node = theContext()->getNode();
    sys_msg->ptr_size = ptr_size;
    // setup envelope
    envelopeInitCopy(sys_msg->env, msg->env);

    vt_debug_print(
      normal, serial_msg,
      "broadcastSerialMsg (non-eager): container: han={}, sys_size={}, "
      "ptr_size={}, total_size={}, group={:x}\n",
      traceable_han, sys_size, ptr_size, total_size, envelopeGetGroup(msg->env)
    );

    using MsgType = SerialWrapperMsgType<MsgT>;
    theMsg()->markAsSerialMsgMessage(sys_msg);
    return theMsg()->broadcastMsgSz<MsgType,serialMsgHandlerBcast>(
      sys_msg, total_size, deliver_to_sender, no_tag
    );
  }
}

template <typename MsgT, typename BaseT>
/*static*/ messaging::PendingSend SerializedMessenger::sendSerialMsgSendImpl(
  MsgT* msg_ptr, HandlerType typed_handler,
  ActionEagerSend<MsgT, BaseT> eager_sender, ActionDataSend data_sender
) {
#ifndef vt_quirked_serialize_method_detection
  static_assert(
    ::vt::messaging::has_own_serialize<MsgT>,
    "Messages sent via SerializedMessenger must have a serialization function."
  );
#endif
  static_assert(
    ::vt::messaging::msg_defines_serialize_mode<MsgT>::value,
    "Messages that define a serialization function must specify serialization mode."
  );
  static_assert(
    ::vt::messaging::msg_serialization_mode<MsgT>::required,
    "Messages sent via SerializedMessenger should require serialization."
  );

  auto msg = promoteMsg(msg_ptr);

  MsgSharedPtr<SerialEagerPayloadMsg<MsgT,BaseT>> payload_msg = nullptr;
  SerialByteType* ptr = nullptr;
  SizeType ptr_size = 0;

  envelopeSetIsLocked(msg->env, true); // implies locked on deserialize
  envelopeSetHasBeenSerialized(msg->env, false);

  auto serialized_msg = checkpoint::serialize(
    *msg.get(), [&](SizeType size) -> SerialByteType* {
      ptr_size = size;

      if (size > serialized_msg_eager_size) {
        ptr = static_cast<SerialByteType*>(std::malloc(size));
        return ptr;
      } else {
        payload_msg = makeMessage<SerialEagerPayloadMsg<MsgT, BaseT>>(
          static_cast<NumBytesType>(ptr_size)
        );
        return payload_msg->payload.data();
      }
    }
  );

  vtAssertInfo(
    envelopeHasBeenSerialized(msg->env),
    "A message was serialized that did not correctly serialize parents."
    " All serialized messages are required to call serialize on the"
    " message's parent type. See 'msg_serialize_parent'.",
    typeid(MsgT).name()
  );

  //fmt::print("ptr_size={}\n", ptr_size);

  vt_debug_print(
    normal, serial_msg,
    "sendSerialMsgHandler: ptr_size={}, han={}, eager={}, epoch={}\n",
    ptr_size, typed_handler, ptr_size <= serialized_msg_eager_size,
    envelopeGetEpoch(msg_ptr->env)
  );

  if (ptr_size > serialized_msg_eager_size) {
    vt_debug_print(
      verbose, serial_msg,
      "sendSerialMsg: non-eager: ptr_size={}\n", ptr_size
    );

    vtAssertExpr(payload_msg == nullptr && data_sender != nullptr);

    auto send_data = [=](NodeType dest) -> messaging::PendingSend {
      auto const& node = theContext()->getNode();
      if (node != dest) {
        auto sys_msg = makeMessage<SerialWrapperMsgType<MsgT>>();
        auto send_serialized = [=](Active::SendFnType send){
          auto ret = send(RDMA_GetType{ptr, ptr_size}, dest, no_tag);
          EventType event = ret.getEvent();
          theEvent()->attachAction(event, [=]{ std::free(ptr); });
          sys_msg->data_recv_tag = ret.getTag();
          sys_msg->nchunks = ret.getNumChunks();
          sys_msg->ptr_size = ptr_size;
        };

        // wrap metadata
        sys_msg->handler = typed_handler;
        sys_msg->from_node = theContext()->getNode();
        // setup envelope
        envelopeInitCopy(sys_msg->env, msg->env);

        vt_debug_print(
          verbose, serial_msg,
          "sendSerialMsg: non-eager: dest={}, sys_msg={}, handler={}\n",
          dest, print_ptr(sys_msg.get()), typed_handler
        );

        theMsg()->markAsSerialMsgMessage(sys_msg);
        auto sys_msg_send = promoteMsg(sys_msg.get()); // payload fn
        return theMsg()->sendMsg<SerialWrapperMsgType<MsgT>, serialMsgHandler>(
          dest, sys_msg_send, send_serialized
        );
      } else {
        // Dest is current node, still runs through serialization; no send.
        auto msg_data = ptr;
        auto user_msg = deserializeFullMessage<MsgT>(msg_data);

        vt_debug_print(
          verbose, serial_msg,
          "serialMsgHandler: local msg: handler={}\n", typed_handler
        );

        auto base_msg = msg.template to<BaseMsgType>();
        return messaging::PendingSend(base_msg, [=](MsgPtr<BaseMsgType> in) {
          runnable::makeRunnable(msg, true, typed_handler, node)
            .withTDEpochFromMsg()
            .enqueue();
        });
      }
    };

    return data_sender(send_data);
  } else {
    vt_debug_print(
      verbose, serial_msg,
      "sendSerialMsg: eager: ptr_size={}\n", ptr_size
    );

    vtAssertExpr(payload_msg != nullptr && eager_sender != nullptr);

    // wrap metadata
    payload_msg->handler = typed_handler;
    payload_msg->from_node = theContext()->getNode();
    // setup envelope
    envelopeInitCopy(payload_msg->env, msg->env);

    return eager_sender(payload_msg);
  }
}

}} /* end namespace vt::serialization */

#endif /*INCLUDED_VT_SERIALIZATION_MESSAGING_SERIALIZED_MESSENGER_IMPL_H*/
