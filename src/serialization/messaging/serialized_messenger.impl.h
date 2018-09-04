
#if !defined INCLUDED_SERIALIZATION_MESSAGING_SERIALIZED_MESSENGER_IMPL_H
#define INCLUDED_SERIALIZATION_MESSAGING_SERIALIZED_MESSENGER_IMPL_H

#include "config.h"
#include "messaging/message.h"
#include "messaging/active.h"
#include "registry/auto/auto_registry_interface.h"
#include "registry/auto/vc/auto_registry_vc.h"
#include "serialization/serialization.h"
#include "runnable/general.h"
#include "serialization/messaging/serialized_data_msg.h"
#include "serialization/messaging/serialized_messenger.h"

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
  messageRef(msg);
  auto const& from_node = theMsg()->getFromNodeCurrentHandler();
  runnable::Runnable<MsgT>::run(user_handler, nullptr, t_ptr, from_node);
  messageDeref(msg);
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

  UserMsgT* user_msg = makeSharedMessage<UserMsgT>();
  auto ptr_offset =
    reinterpret_cast<char*>(sys_msg) + sizeof(SerialWrapperMsgType<UserMsgT>);
  auto t_ptr = deserialize<UserMsgT>(ptr_offset, ptr_size, user_msg);

  messageRef(user_msg);
  runnable::Runnable<UserMsgT>::run(
    handler, nullptr, t_ptr, sys_msg->from_node
  );
  messageDeref(user_msg);
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
    "recv_tag={}\n",
    print_ptr(sys_msg), handler, recv_tag
  );

  auto node = sys_msg->from_node;
  theMsg()->recvDataMsg(
    recv_tag, sys_msg->from_node,
    [handler,recv_tag,node](RDMA_GetType ptr, ActionType){
      // be careful here not to use "msg", it is no longer valid
      auto raw_ptr = reinterpret_cast<SerialByteType*>(std::get<0>(ptr));
      auto ptr_size = std::get<1>(ptr);
      //UserMsgT* msg = static_cast<UserMsgT*>(std::malloc(sizeof(UserMsgT)));
      UserMsgT* msg = makeSharedMessage<UserMsgT>();
      auto tptr = deserialize<UserMsgT>(raw_ptr, ptr_size, msg);

      debug_print(
        serial_msg, node,
        "serialMsgHandler: recvDataMsg finished: handler={}, recv_tag={}\n",
        handler, recv_tag
      );

      messageRef(msg);
      runnable::Runnable<UserMsgT>::run(handler, nullptr, tptr, node);
      messageDeref(msg);
      //std::free(msg);
    }
  );
}

template <typename UserMsgT, typename BaseEagerMsgT>
/*static*/ void SerializedMessenger::payloadMsgHandler(
  SerialEagerPayloadMsg<UserMsgT, BaseEagerMsgT>* sys_msg
) {
  auto const handler = sys_msg->handler;

  //UserMsgT* user_msg = static_cast<UserMsgT*>(std::malloc(sizeof(UserMsgT)));
  UserMsgT* user_msg = makeSharedMessage<UserMsgT>();
  auto tptr = deserialize<UserMsgT>(
    sys_msg->payload.data(), sys_msg->bytes, user_msg
  );
  auto const& group_ = envelopeGetGroup(sys_msg->env);

  debug_print(
    serial_msg, node,
    "payloadMsgHandler: group={:x}, msg={}, handler={}, bytes={}\n",
    group_, print_ptr(sys_msg), handler, sys_msg->bytes
  );

  messageRef(user_msg);
  runnable::Runnable<UserMsgT>::run(
    handler, nullptr, tptr, sys_msg->from_node
  );
  messageDeref(user_msg);
  //std::free(user_msg);
}

template <typename MsgT, ActiveTypedFnType<MsgT> *f, typename BaseT>
/*static*/ void SerializedMessenger::sendParserdesMsg(
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
/*static*/ void SerializedMessenger::sendParserdesMsg(
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
/*static*/ void SerializedMessenger::sendParserdesMsgHandler(
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
/*static*/ void SerializedMessenger::broadcastParserdesMsg(MsgT* msg) {
  debug_print(
    serial_msg, node,
    "broadcastParserdesMsg: (functor) msg={}\n", print_ptr(msg)
  );
  return parserdesMsg<FunctorT,MsgT>(msg,true);
}

template <typename MsgT, ActiveTypedFnType<MsgT> *f, typename BaseT>
/*static*/ void SerializedMessenger::broadcastParserdesMsg(MsgT* msg) {
  debug_print(
    serial_msg, node,
    "broadcastParserdesMsg: msg={}\n", print_ptr(msg)
  );
  return parserdesMsg<MsgT,f>(msg,true);
}

template <typename MsgT, typename BaseT>
/*static*/ void SerializedMessenger::broadcastParserdesMsgHandler(
  MsgT* msg, HandlerType const& han
) {
  debug_print(
    serial_msg, node,
    "broadcastParserdesMsgHandler: msg={}, han={}\n", print_ptr(msg), han
  );
  return parserdesMsgHandler<MsgT>(msg,han,true);
}


template <typename FunctorT, typename MsgT, typename BaseT>
/*static*/ void SerializedMessenger::parserdesMsg(
  MsgT* msg, bool is_bcast, NodeType dest
) {
  auto const& h = auto_registry::makeAutoHandlerFunctor<FunctorT,true,MsgT*>();
  return parserdesMsgHandler(msg,h,is_bcast,dest);
}

template <typename MsgT, ActiveTypedFnType<MsgT> *f, typename BaseT>
/*static*/ void SerializedMessenger::parserdesMsg(
  MsgT* msg, bool is_bcast, NodeType dest
) {
  auto const& h = auto_registry::makeAutoHandler<MsgT,f>(nullptr);
  return parserdesMsgHandler(msg,h,is_bcast,dest);
}

template <typename MsgT, typename BaseT>
/*static*/ void SerializedMessenger::parserdesMsgHandler(
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
        assert(0 && "Must fit in remaining size (current limitation)");
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
  auto const& act = no_action;
  if (is_bcast) {
    theMsg()->broadcastMsgSz<MsgT,parserdesHandler>(msg, total_size, tag, act);
  } else {
    theMsg()->sendMsgSz<MsgT,parserdesHandler>(dest, msg, total_size, tag, act);
  }
}

template <typename MsgT, ActiveTypedFnType<MsgT> *f, typename BaseT>
/*static*/ void SerializedMessenger::sendSerialMsg(
  NodeType dest, MsgT* msg, ActionEagerSend<MsgT, BaseT> eager
) {
  auto const& h = auto_registry::makeAutoHandler<MsgT,f>(nullptr);
  return sendSerialMsgHandler<MsgT,BaseT>(dest,msg,h,eager);
}

template <typename FunctorT, typename MsgT, typename BaseT>
/*static*/ void SerializedMessenger::sendSerialMsg(
  NodeType dest, MsgT* msg, ActionEagerSend<MsgT, BaseT> eager
) {
  auto const& h = auto_registry::makeAutoHandlerFunctor<FunctorT,true,MsgT*>();
  return sendSerialMsgHandler<MsgT,BaseT>(dest,msg,h,eager);
}

template <typename MsgT, typename BaseT>
/*static*/ void SerializedMessenger::sendSerialMsgHandler(
  NodeType dest, MsgT* msg, HandlerType const& handler,
  ActionEagerSend<MsgT, BaseT> eager_sender
) {
  auto eager_default_send = [=](SerializedEagerMsg<MsgT,BaseT>* m){
    using MsgType = SerialEagerPayloadMsg<MsgT,BaseT>;
    theMsg()->sendMsg<MsgType,payloadMsgHandler>(dest,m);
  };
  auto eager = eager_sender ? eager_sender : eager_default_send;
  return sendSerialMsgHandler<MsgT,BaseT>(
    msg,eager,handler,[=](ActionNodeType action) {
      action(dest);
    }
  );
}

template <typename FunctorT, typename MsgT, typename BaseT>
/*static*/ void SerializedMessenger::broadcastSerialMsg(MsgT* msg) {
  auto const& h = auto_registry::makeAutoHandlerFunctor<FunctorT,true,MsgT*>();
  return broadcastSerialMsgHandler<MsgT,BaseT>(msg,h);
}

template <typename MsgT, ActiveTypedFnType<MsgT> *f, typename BaseT>
/*static*/ void SerializedMessenger::broadcastSerialMsg(MsgT* msg) {
  auto const& h = auto_registry::makeAutoHandler<MsgT,f>(nullptr);
  return broadcastSerialMsgHandler<MsgT,BaseT>(msg,h);
}

template <typename MsgT, typename BaseT>
/*static*/ void SerializedMessenger::broadcastSerialMsgHandler(
  MsgT* msg, HandlerType const& han
) {
  using PayloadMsg = SerialEagerPayloadMsg<MsgT, BaseT>;

  SerialEagerPayloadMsg<MsgT, BaseT>* payload_msg = nullptr;
  SizeType ptr_size = 0;
  SerialWrapperMsgType<MsgT>* sys_msg = nullptr;
  auto const& sys_size = sizeof(SerialWrapperMsgType<MsgT>);

  auto serialized_msg = serialize(
    *msg, [&](SizeType size) -> SerialByteType* {
      ptr_size = size;
      if (size >= serialized_msg_eager_size) {
        char* buf = static_cast<char*>(thePool()->alloc(ptr_size + sys_size));
        sys_msg = new (buf) SerialWrapperMsgType<MsgT>{};
        return buf + sys_size;
      } else {
        payload_msg = makeSharedMessage<PayloadMsg>(
          static_cast<NumBytesType>(size)
        );
        return payload_msg->payload.data();
      }
    }
  );

  auto const& group_ = envelopeGetGroup(msg->env);

  if (ptr_size < serialized_msg_eager_size) {
    assert(
      ptr_size < serialized_msg_eager_size &&
      "Must be smaller for eager protocol"
    );

    // move serialized msg envelope to system envelope to preserve info
    payload_msg->env = msg->env;
    payload_msg->handler = han;
    payload_msg->from_node = theContext()->getNode();
    envelopeSetGroup(payload_msg->env, group_);

    debug_print(
      serial_msg, node,
      "broadcastSerialMsg: han={}, size={}, serialized_msg_eager_size={}, "
      "group={:x}\n",
      han, ptr_size, serialized_msg_eager_size, group_
    );

    theMsg()->broadcastMsg<PayloadMsg,payloadMsgHandler>(payload_msg);
  } else {
    auto const& total_size = ptr_size + sys_size;

    sys_msg->handler = han;
    sys_msg->from_node = theContext()->getNode();
    sys_msg->ptr_size = ptr_size;
    envelopeSetGroup(sys_msg->env, group_);

    debug_print(
      serial_msg, node,
      "broadcastSerialMsg: container: han={}, sys_size={}, ptr_size={}, "
      "total_size={}, group={:x}\n",
      han, sys_size, ptr_size, total_size, group_
    );

    theMsg()->broadcastMsgSz<SerialWrapperMsgType<MsgT>,serialMsgHandlerBcast>(
      sys_msg, total_size, no_tag, no_action
    );
  }
}

template <typename MsgT, ActiveTypedFnType<MsgT> *f, typename BaseT>
/*static*/ void SerializedMessenger::sendSerialMsg(
  MsgT* msg, ActionEagerSend<MsgT, BaseT> eager, ActionDataSend sender
) {
  auto const& h = auto_registry::makeAutoHandler<MsgT,f>(nullptr);
  return sendSerialMsgHandler<MsgT,BaseT>(msg,eager,h,sender);
}

template <typename MsgT, typename BaseT>
/*static*/ void SerializedMessenger::sendSerialMsgHandler(
  MsgT* msg, ActionEagerSend<MsgT, BaseT> eager_sender,
  HandlerType const& typed_handler, ActionDataSend data_sender
) {
  SerialEagerPayloadMsg<MsgT, BaseT>* payload_msg = nullptr;
  SerialByteType* ptr = nullptr;
  SizeType ptr_size = 0;

  auto serialized_msg = serialize(
    *msg, [&](SizeType size) -> SerialByteType* {
      ptr_size = size;

      if (size > serialized_msg_eager_size) {
        ptr = static_cast<SerialByteType*>(malloc(size));
        return ptr;
      } else {
        payload_msg = makeSharedMessage<SerialEagerPayloadMsg<MsgT, BaseT>>(
          static_cast<NumBytesType>(ptr_size)
        );
        return payload_msg->payload.data();
      }
    }
  );

  //fmt::print("ptr_size={}\n", ptr_size);

  debug_print(
    serial_msg, node,
    "sendSerialMsgHandler: ptr_size={}, han={}, eager={}\n",
    ptr_size, typed_handler, ptr_size <= serialized_msg_eager_size
  );

  if (ptr_size > serialized_msg_eager_size) {
    debug_print(
      serial_msg, node,
      "sendSerialMsg: non-eager: ptr_size={}\n", ptr_size
    );

    assert(payload_msg == nullptr && data_sender != nullptr);

    auto send_data = [=](NodeType dest){
      auto const& node = theContext()->getNode();
      if (node != dest) {
        auto sys_msg = makeSharedMessage<SerialWrapperMsgType<MsgT>>();
        auto send_serialized = [&](Active::SendFnType send){
          auto ret = send(RDMA_GetType{ptr, ptr_size}, dest, no_tag, no_action);
          sys_msg->data_recv_tag = std::get<1>(ret);
        };

        sys_msg->env = msg->env;
        sys_msg->handler = typed_handler;
        sys_msg->from_node = theContext()->getNode();

        debug_print(
          serial_msg, node,
          "sendSerialMsg: non-eager: dest={}, sys_msg={}, handler={}\n",
          dest, print_ptr(sys_msg), typed_handler
        );

        theMsg()->sendMsg<SerialWrapperMsgType<MsgT>, serialMsgHandler>(
          dest, sys_msg, send_serialized
        );
      } else {
        //MsgT* msg = static_cast<MsgT*>(std::malloc(sizeof(MsgT)));
        MsgT* msg = makeSharedMessage<MsgT>();
        auto tptr = deserialize<MsgT>(ptr, ptr_size, msg);

        debug_print(
          serial_msg, node,
          "serialMsgHandler: local msg: handler={}\n", typed_handler
        );

        messageRef(msg);
        runnable::Runnable<MsgT>::run(typed_handler,nullptr,tptr,node);
        messageDeref(msg);
        //std::free(msg);
      }
    };

    data_sender(send_data);
  } else {
    debug_print(
      serial_msg, node,
      "sendSerialMsg: eager: ptr_size={}\n", ptr_size
    );

    assert(payload_msg != nullptr and eager_sender != nullptr);

    // move serialized msg envelope to system envelope to preserve info
    payload_msg->env = msg->env;
    payload_msg->handler = typed_handler;
    payload_msg->from_node = theContext()->getNode();

    eager_sender(payload_msg);
  }
}

}} /* end namespace vt::serialization */

#endif /*INCLUDED_SERIALIZATION_MESSAGING_SERIALIZED_MESSENGER_IMPL_H*/
