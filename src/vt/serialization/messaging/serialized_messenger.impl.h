/*
//@HEADER
// ************************************************************************
//
//                          serialized_messenger.impl.h
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

#if !defined INCLUDED_SERIALIZATION_MESSAGING_SERIALIZED_MESSENGER_IMPL_H
#define INCLUDED_SERIALIZATION_MESSAGING_SERIALIZED_MESSENGER_IMPL_H

#include "vt/config.h"
#include "vt/messaging/message.h"
#include "vt/messaging/active.h"
#include "vt/registry/auto/auto_registry_interface.h"
#include "vt/registry/auto/vc/auto_registry_vc.h"
#include "vt/serialization/serialization.h"
#include "vt/runnable/general.h"
#include "vt/serialization/messaging/serialized_data_msg.h"
#include "vt/serialization/messaging/serialized_messenger.h"

#include <tuple>
#include <type_traits>
#include <cstdlib>
#include <cassert>

namespace vt { namespace serialization {

template <typename MsgT>
/*static*/ void SerializedMessenger::parserdesHandler(MsgT* msg) {
  auto const& msg_size = sizeof(MsgT);
  auto const& han_size = sizeof(HandlerType);
  auto const& size_size = sizeof(size_t);
  auto msg_ptr = reinterpret_cast<char*>(msg);
  auto const& user_handler = *reinterpret_cast<HandlerType*>(
    msg_ptr + msg_size
  );
  auto const& ptr_size = *reinterpret_cast<size_t*>(
    msg_ptr + msg_size + han_size
  );
  auto ptr_offset = msg_ptr + msg_size + han_size + size_size;
  auto t_ptr = deserializePartial<MsgT>(ptr_offset, ptr_size, msg);
  auto const& from_node = theMsg()->getFromNodeCurrentHandler();
  runnable::Runnable<MsgT>::run(user_handler, nullptr, t_ptr, from_node);
}

template <typename UserMsgT>
/*static*/ void SerializedMessenger::serialMsgHandlerBcast(
  SerialWrapperMsgType<UserMsgT>* sys_msg
) {
  auto const& handler = sys_msg->handler;
  auto const& ptr_size = sys_msg->ptr_size;
  auto const& group_ = envelopeGetGroup(sys_msg->env);

  debug_print(
    serial_msg, node,
    "serialMsgHandlerBcast: group_={:x}, handler={}, ptr_size={}\n",
    group_, handler, ptr_size
  );

  auto ptr_offset =
    reinterpret_cast<char*>(sys_msg) + sizeof(SerialWrapperMsgType<UserMsgT>);
  auto user_msg = makeMessage<UserMsgT>();
  deserializeInPlace<UserMsgT>(ptr_offset,ptr_size,user_msg.get());
  messageResetDeserdes(user_msg);
  runnable::Runnable<UserMsgT>::run(
    handler, nullptr, user_msg.get(), sys_msg->from_node
  );
}

template <typename UserMsgT>
/*static*/ void SerializedMessenger::serialMsgHandler(
  SerialWrapperMsgType<UserMsgT>* sys_msg
) {
  auto const handler = sys_msg->handler;
  auto const& recv_tag = sys_msg->data_recv_tag;

  debug_print(
    serial_msg, node,
    "serialMsgHandler: non-eager, recvDataMsg: msg={}, handler={}, "
    "recv_tag={}, epoch={}\n",
    print_ptr(sys_msg), handler, recv_tag, envelopeGetEpoch(sys_msg->env)
  );

  auto node = sys_msg->from_node;
  theMsg()->recvDataMsg(
    recv_tag, sys_msg->from_node,
    [handler,recv_tag,node](RDMA_GetType ptr, ActionType action){
      // be careful here not to use "msg", it is no longer valid
      auto raw_ptr = reinterpret_cast<SerialByteType*>(std::get<0>(ptr));
      auto ptr_size = std::get<1>(ptr);
      auto msg = makeMessage<UserMsgT>();
      deserializeInPlace<UserMsgT>(raw_ptr, ptr_size, msg.get());
      messageResetDeserdes(msg);

      debug_print(
        serial_msg, node,
        "serialMsgHandler: recvDataMsg finished: handler={}, recv_tag={},"
        "epoch={}\n",
        handler, recv_tag, envelopeGetEpoch(msg->env)
      );

      runnable::Runnable<UserMsgT>::run(handler, nullptr, msg.get(), node);
      action();
    }
  );
}

template <typename UserMsgT, typename BaseEagerMsgT>
/*static*/ void SerializedMessenger::payloadMsgHandler(
  SerialEagerPayloadMsg<UserMsgT, BaseEagerMsgT>* sys_msg
) {
  auto const handler = sys_msg->handler;

  auto user_msg = makeMessage<UserMsgT>();
  deserializeInPlace<UserMsgT>(
    sys_msg->payload.data(), sys_msg->bytes, user_msg.get()
  );
  messageResetDeserdes(user_msg);

  auto const& group_ = envelopeGetGroup(sys_msg->env);

  debug_print(
    serial_msg, node,
    "payloadMsgHandler: group={:x}, msg={}, handler={}, bytes={}, "
    "user ref={}, sys ref={}, user_msg={}, epoch={}\n",
    group_, print_ptr(sys_msg), handler, sys_msg->bytes,
    envelopeGetRef(user_msg->env), envelopeGetRef(sys_msg->env),
    print_ptr(user_msg.get()), envelopeGetEpoch(sys_msg->env)
  );

  runnable::Runnable<UserMsgT>::run(
    handler, nullptr, user_msg.get(), sys_msg->from_node
  );
}

template <typename MsgT, ActiveTypedFnType<MsgT> *f, typename BaseT>
/*static*/ messaging::PendingSend SerializedMessenger::sendParserdesMsg(
  NodeType dest, MsgT* msg
) {
  debug_print(
    serial_msg, node,
    "sendParserdesMsg: dest={}, msg={}\n",
    dest, print_ptr(msg)
  );
  return parserdesMsg<MsgT,f>(msg, false, dest);
}

template <typename FunctorT, typename MsgT, typename BaseT>
/*static*/ messaging::PendingSend SerializedMessenger::sendParserdesMsg(
  NodeType dest, MsgT* msg
) {
  debug_print(
    serial_msg, node,
    "sendParserdesMsg: (functor) dest={}, msg={}\n",
    dest, print_ptr(msg)
  );
  return parserdesMsg<FunctorT,MsgT>(msg, false, dest);
}

template <typename MsgT, typename BaseT>
/*static*/ messaging::PendingSend SerializedMessenger::sendParserdesMsgHandler(
  NodeType dest, HandlerType const& handler, MsgT* msg
) {
  debug_print(
    serial_msg, node,
    "sendParserdesMsg: dest={}, msg={}\n",
    dest, print_ptr(msg)
  );
  return parserdesMsgHandler<MsgT>(msg, handler, false, dest);
}

template <typename FunctorT, typename MsgT, typename BaseT>
/*static*/ messaging::PendingSend
 SerializedMessenger::broadcastParserdesMsg(MsgT* msg) {
  debug_print(
    serial_msg, node,
    "broadcastParserdesMsg: (functor) msg={}\n", print_ptr(msg)
  );
  return parserdesMsg<FunctorT,MsgT>(msg,true);
}

template <typename MsgT, ActiveTypedFnType<MsgT> *f, typename BaseT>
/*static*/ messaging::PendingSend
 SerializedMessenger::broadcastParserdesMsg(MsgT* msg) {
  debug_print(
    serial_msg, node,
    "broadcastParserdesMsg: msg={}\n", print_ptr(msg)
  );
  return parserdesMsg<MsgT,f>(msg,true);
}

template <typename MsgT, typename BaseT>
/*static*/ messaging::PendingSend
 SerializedMessenger::broadcastParserdesMsgHandler(
  MsgT* msg, HandlerType const& han
) {
  debug_print(
    serial_msg, node,
    "broadcastParserdesMsgHandler: msg={}, han={}\n", print_ptr(msg), han
  );
  return parserdesMsgHandler<MsgT>(msg,han,true);
}


template <typename FunctorT, typename MsgT, typename BaseT>
/*static*/ messaging::PendingSend SerializedMessenger::parserdesMsg(
  MsgT* msg, bool is_bcast, NodeType dest
) {
  auto const& h = auto_registry::makeAutoHandlerFunctor<FunctorT,true,MsgT*>();
  return parserdesMsgHandler(msg,h,is_bcast,dest);
}

template <typename MsgT, ActiveTypedFnType<MsgT> *f, typename BaseT>
/*static*/ messaging::PendingSend SerializedMessenger::parserdesMsg(
  MsgT* msg, bool is_bcast, NodeType dest
) {
  auto const& h = auto_registry::makeAutoHandler<MsgT,f>(nullptr);
  return parserdesMsgHandler(msg,h,is_bcast,dest);
}

template <typename MsgT, typename BaseT>
/*static*/ messaging::PendingSend SerializedMessenger::parserdesMsgHandler(
  MsgT* msg, HandlerType const& handler, bool is_bcast, NodeType dest
) {
  auto const& user_handler = handler;
  SizeType ptr_size = 0;
  auto const& rem_size = thePool()->remainingSize(msg);
  auto const& msg_size = sizeof(MsgT);
  auto const& han_size = sizeof(HandlerType);
  auto const& size_size = sizeof(size_t);
  auto msg_ptr = reinterpret_cast<char*>(msg);

  auto serialized_msg = serializePartial(
    *msg, [&](SizeType size) -> SerialByteType* {
      ptr_size = size;
      if (size + han_size <= rem_size) {
        auto ptr = msg_ptr + msg_size + han_size + size_size;
        return ptr;
      } else {
        vtAssert(0, "Must fit in remaining size (current limitation)");
        return nullptr;
      }
    }
  );

  debug_print(
    serial_msg, node,
    "parserdesMsg: ptr_size={}, rem_size={}, msg_size={}, is_bcast={}, "
    "dest={}\n",
    ptr_size, rem_size, msg_size, is_bcast, dest
  );

  *reinterpret_cast<HandlerType*>(msg_ptr + msg_size) = user_handler;
  *reinterpret_cast<HandlerType*>(msg_ptr + msg_size + han_size) = ptr_size;

  auto const& total_size = ptr_size + msg_size + han_size + size_size;
  auto const& tag = no_tag;
  if (is_bcast) {
    return theMsg()->broadcastMsgSz<MsgT,parserdesHandler>(msg, total_size, tag);
  } else {
    return theMsg()->sendMsgSz<MsgT,parserdesHandler>(dest, msg, total_size, tag);
  }
}

template <typename MsgT, ActiveTypedFnType<MsgT> *f, typename BaseT>
/*static*/ messaging::PendingSend SerializedMessenger::sendSerialMsg(
  NodeType dest, MsgT* msg, ActionEagerSend<MsgT, BaseT> eager
) {
  auto const& h = auto_registry::makeAutoHandler<MsgT,f>(nullptr);
  return sendSerialMsgHandler<MsgT,BaseT>(dest,msg,h,eager);
}

template <typename FunctorT, typename MsgT, typename BaseT>
/*static*/ messaging::PendingSend SerializedMessenger::sendSerialMsg(
  NodeType dest, MsgT* msg, ActionEagerSend<MsgT, BaseT> eager
) {
  auto const& h = auto_registry::makeAutoHandlerFunctor<FunctorT,true,MsgT*>();
  return sendSerialMsgHandler<MsgT,BaseT>(dest,msg,h,eager);
}

template <typename MsgT, typename BaseT>
/*static*/ messaging::PendingSend SerializedMessenger::sendSerialMsgHandler(
  NodeType dest, MsgT* msg, HandlerType const& handler,
  ActionEagerSend<MsgT, BaseT> eager_sender
) {
  auto eager_default_send =
    [=](MsgSharedPtr<SerializedEagerMsg<MsgT,BaseT>> m) -> messaging::PendingSend {
    using MsgType = SerialEagerPayloadMsg<MsgT,BaseT>;
    return theMsg()->sendMsg<MsgType,payloadMsgHandler>(dest,m.get());
  };
  auto eager = eager_sender ? eager_sender : eager_default_send;
  return sendSerialMsgHandler<MsgT,BaseT>(
    msg,eager,handler,[=](ActionNodeSendType action) -> messaging::PendingSend {
      return action(dest);
    }
  );
}

template <typename FunctorT, typename MsgT, typename BaseT>
/*static*/ messaging::PendingSend
 SerializedMessenger::broadcastSerialMsg(MsgT* msg) {
  auto const& h = auto_registry::makeAutoHandlerFunctor<FunctorT,true,MsgT*>();
  return broadcastSerialMsgHandler<MsgT,BaseT>(msg,h);
}

template <typename MsgT, ActiveTypedFnType<MsgT> *f, typename BaseT>
/*static*/ messaging::PendingSend
 SerializedMessenger::broadcastSerialMsg(MsgT* msg) {
  auto const& h = auto_registry::makeAutoHandler<MsgT,f>(nullptr);
  return broadcastSerialMsgHandler<MsgT,BaseT>(msg,h);
}

template <typename MsgT, typename BaseT>
/*static*/ messaging::PendingSend
 SerializedMessenger::broadcastSerialMsgHandler(
  MsgT* msg_ptr, HandlerType const& han
) {
  using PayloadMsg = SerialEagerPayloadMsg<MsgT, BaseT>;

  auto msg = promoteMsg(msg_ptr);

  MsgSharedPtr<SerialEagerPayloadMsg<MsgT,BaseT>> payload_msg = nullptr;
  MsgSharedPtr<SerialWrapperMsgType<MsgT>> sys_msg = nullptr;
  SizeType ptr_size = 0;
  auto sys_size = sizeof(typename decltype(sys_msg)::MsgType);

  auto serialized_msg = serialize(
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

  auto const& group_ = envelopeGetGroup(msg->env);

  if (ptr_size < serialized_msg_eager_size) {
    vtAssert(
      ptr_size < serialized_msg_eager_size,
      "Must be smaller for eager protocol"
    );

    // move serialized msg envelope to system envelope to preserve info
    auto cur_ref = envelopeGetRef(payload_msg->env);
    payload_msg->env = msg->env;
    payload_msg->handler = han;
    payload_msg->from_node = theContext()->getNode();
    envelopeSetGroup(payload_msg->env, group_);
    envelopeSetRef(payload_msg->env, cur_ref);

    debug_print(
      serial_msg, node,
      "broadcastSerialMsg (eager): han={}, size={}, "
      "serialized_msg_eager_size={}, group={:x}\n",
      han, ptr_size, serialized_msg_eager_size, group_
    );

    return theMsg()->broadcastMsg<PayloadMsg,payloadMsgHandler>(
      payload_msg.get()
    );
  } else {
    auto const& total_size = ptr_size + sys_size;

    sys_msg->handler = han;
    sys_msg->env = msg->env;
    sys_msg->from_node = theContext()->getNode();
    sys_msg->ptr_size = ptr_size;
    envelopeSetGroup(sys_msg->env, group_);

    debug_print(
      serial_msg, node,
      "broadcastSerialMsg (non-eager): container: han={}, sys_size={}, "
      "ptr_size={}, total_size={}, group={:x}\n",
      han, sys_size, ptr_size, total_size, group_
    );

    using MsgType = SerialWrapperMsgType<MsgT>;
    return theMsg()->broadcastMsgSz<MsgType,serialMsgHandlerBcast>(
      sys_msg.get(), total_size, no_tag
    );
  }
}

template <typename MsgT, ActiveTypedFnType<MsgT> *f, typename BaseT>
/*static*/ messaging::PendingSend SerializedMessenger::sendSerialMsg(
  MsgT* msg, ActionEagerSend<MsgT, BaseT> eager, ActionDataSend sender
) {
  auto const& h = auto_registry::makeAutoHandler<MsgT,f>(nullptr);
  return sendSerialMsgHandler<MsgT,BaseT>(msg,eager,h,sender);
}

template <typename MsgT, typename BaseT>
/*static*/ messaging::PendingSend SerializedMessenger::sendSerialMsgHandler(
  MsgT* msg_ptr, ActionEagerSend<MsgT, BaseT> eager_sender,
  HandlerType const& typed_handler, ActionDataSend data_sender
) {
  auto msg = promoteMsg(msg_ptr);

  MsgSharedPtr<SerialEagerPayloadMsg<MsgT,BaseT>> payload_msg = nullptr;
  SerialByteType* ptr = nullptr;
  SizeType ptr_size = 0;

  auto serialized_msg = serialize(
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

  //fmt::print("ptr_size={}\n", ptr_size);

  debug_print(
    serial_msg, node,
    "sendSerialMsgHandler: ptr_size={}, han={}, eager={}, epoch={}\n",
    ptr_size, typed_handler, ptr_size <= serialized_msg_eager_size,
    envelopeGetEpoch(msg_ptr->env)
  );

  if (ptr_size > serialized_msg_eager_size) {
    debug_print(
      serial_msg, node,
      "sendSerialMsg: non-eager: ptr_size={}\n", ptr_size
    );

    vtAssertExpr(payload_msg == nullptr && data_sender != nullptr);

    auto send_data = [=](NodeType dest) -> messaging::PendingSend {
      auto const& node = theContext()->getNode();
      if (node != dest) {
        auto sys_msg = makeMessage<SerialWrapperMsgType<MsgT>>();
        auto send_serialized = [=](Active::SendFnType send){
          auto ret = send(RDMA_GetType{ptr, ptr_size}, dest, no_tag);
	  EventType event = std::get<0>(ret);
	  theEvent()->attachAction(event, [=]{ std::free(ptr); });
          sys_msg->data_recv_tag = std::get<1>(ret);
        };
        auto cur_ref = envelopeGetRef(sys_msg->env);
        sys_msg->env = msg->env;
        sys_msg->handler = typed_handler;
        sys_msg->from_node = theContext()->getNode();
        envelopeSetRef(sys_msg->env, cur_ref);

        debug_print(
          serial_msg, node,
          "sendSerialMsg: non-eager: dest={}, sys_msg={}, handler={}\n",
          dest, print_ptr(sys_msg.get()), typed_handler
        );

        return theMsg()->sendMsg<SerialWrapperMsgType<MsgT>, serialMsgHandler>(
          dest, sys_msg.get(), send_serialized
        );
      } else {
        auto user_msg = makeMessage<MsgT>();
        deserializeInPlace<MsgT>(ptr, ptr_size, user_msg.get());
        messageResetDeserdes(user_msg);

        debug_print(
          serial_msg, node,
          "serialMsgHandler: local msg: handler={}\n", typed_handler
        );

        return messaging::PendingSend(msg, [=](MsgVirtualPtr<BaseMsgType> in){
          runnable::Runnable<MsgT>::run(typed_handler,nullptr,msg.get(),node);
        });
      }
    };

    return data_sender(send_data);
  } else {
    debug_print(
      serial_msg, node,
      "sendSerialMsg: eager: ptr_size={}\n", ptr_size
    );

    vtAssertExpr(payload_msg != nullptr && eager_sender != nullptr);

    // move serialized msg envelope to system envelope to preserve info
    auto cur_ref = envelopeGetRef(payload_msg->env);
    payload_msg->env = msg->env;
    payload_msg->handler = typed_handler;
    payload_msg->from_node = theContext()->getNode();
    envelopeSetRef(payload_msg->env, cur_ref);

    return eager_sender(payload_msg);
  }
}

}} /* end namespace vt::serialization */

#endif /*INCLUDED_SERIALIZATION_MESSAGING_SERIALIZED_MESSENGER_IMPL_H*/
