
#if !defined INCLUDED_SERIALIZATION_MESSAGING_SERIALIZED_MESSENGER_H
#define INCLUDED_SERIALIZATION_MESSAGING_SERIALIZED_MESSENGER_H

#include "config.h"
#include "messaging/message.h"
#include "messaging/active.h"
#include "registry/auto/auto_registry_interface.h"
#include "registry/auto/vc/auto_registry_vc.h"
#include "serialization/serialization.h"
#include "serialization/messaging/serialized_data_msg.h"

#include <tuple>
#include <type_traits>
#include <cstdlib>

using namespace ::serialization::interface;

namespace vt { namespace serialization {

template <typename MsgT, typename BaseEagerMsgT>
using SerializedEagerMsg = SerialEagerPayloadMsg<MsgT, BaseEagerMsgT>;

template <typename MsgT, typename BaseEagerMsgT>
using ActionEagerSend = std::function<void(
  SerializedEagerMsg<MsgT, BaseEagerMsgT>* msg
)>;
using ActionDataSend = std::function<void(ActionNodeType)>;

struct SerializedMessenger {
  template <typename UserMsgT>
  using SerialWrapperMsgType = SerializedDataMsg<UserMsgT>;

  template <typename MsgT>
  static void parserdesHandler(MsgT* msg) {
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
    auto active_fn = auto_registry::getAutoHandler(user_handler);
    active_fn(reinterpret_cast<BaseMessage*>(t_ptr));
    messageDeref(msg);
  }

  template <typename UserMsgT>
  static void serialMsgHandlerBcast(SerialWrapperMsgType<UserMsgT>* sys_msg) {
    auto const& handler = sys_msg->handler;
    auto const& ptr_size = sys_msg->ptr_size;

    debug_print(
      serial_msg, node,
      "serialMsgHandlerBcast: handler={}, ptr_size={}\n",
      handler, ptr_size
    );

    UserMsgT* user_msg = makeSharedMessage<UserMsgT>();
    auto ptr_offset =
      reinterpret_cast<char*>(sys_msg) + sizeof(SerialWrapperMsgType<UserMsgT>);
    auto t_ptr = deserialize<UserMsgT>(ptr_offset, ptr_size, user_msg);

    messageRef(user_msg);
    auto active_fn = auto_registry::getAutoHandler(handler);
    active_fn(reinterpret_cast<BaseMessage*>(t_ptr));
    messageDeref(user_msg);
  }

  template <typename UserMsgT>
  static void serialMsgHandler(SerialWrapperMsgType<UserMsgT>* sys_msg) {
    auto const handler = sys_msg->handler;
    auto const& recv_tag = sys_msg->data_recv_tag;

    debug_print(
      serial_msg, node,
      "serialMsgHandler: non-eager, recvDataMsg: msg={}, handler={}, "
      "recv_tag={}\n",
      print_ptr(sys_msg), handler, recv_tag
    );

    theMsg()->recvDataMsg(
      recv_tag, sys_msg->from_node, [handler,recv_tag](RDMA_GetType ptr, ActionType){
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
        auto active_fn = auto_registry::getAutoHandler(handler);
        active_fn(reinterpret_cast<BaseMessage*>(tptr));
        messageDeref(msg);
        //std::free(msg);
      }
    );
  }

  template <typename UserMsgT, typename BaseEagerMsgT>
  static void payloadMsgHandler(
    SerialEagerPayloadMsg<UserMsgT, BaseEagerMsgT>* sys_msg
  ) {
    auto const handler = sys_msg->handler;

    //UserMsgT* user_msg = static_cast<UserMsgT*>(std::malloc(sizeof(UserMsgT)));
    UserMsgT* user_msg = makeSharedMessage<UserMsgT>();
    auto tptr = deserialize<UserMsgT>(
      sys_msg->payload.data(), sys_msg->bytes, user_msg
    );

    debug_print(
      serial_msg, node,
      "payloadMsgHandler: msg={}, handler={}, bytes={}\n",
      print_ptr(sys_msg), handler, sys_msg->bytes
    );

    messageRef(user_msg);
    auto active_fn = auto_registry::getAutoHandler(handler);
    active_fn(reinterpret_cast<BaseMessage*>(tptr));
    messageDeref(user_msg);
    //std::free(user_msg);
  }

  template <typename MsgT, ActiveTypedFnType<MsgT> *f, typename BaseT = Message>
  static void sendParserdesMsg(NodeType dest, MsgT* msg) {
    debug_print(
      serial_msg, node,
      "sendParserdesMsg: dest={}, msg={}\n",
      dest, print_ptr(msg)
    );
    return parserdesMsg<MsgT,f>(msg, false, dest);
  }

  template <typename MsgT, ActiveTypedFnType<MsgT> *f, typename BaseT = Message>
  static void broadcastParserdesMsg(MsgT* msg) {
    debug_print(
      serial_msg, node,
      "broadcastParserdesMsg: msg={}\n", print_ptr(msg)
    );
    return parserdesMsg<MsgT,f>(msg);
  }

  template <typename MsgT, ActiveTypedFnType<MsgT> *f, typename BaseT = Message>
  static void parserdesMsg(
    MsgT* msg, bool is_bcast = true, NodeType dest = uninitialized_destination
  ) {
    HandlerType const& user_handler =
      auto_registry::makeAutoHandler<MsgT, f>(nullptr);

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

  template <typename MsgT, ActiveTypedFnType<MsgT> *f, typename BaseT = Message>
  static void sendSerialMsg(
    NodeType dest, MsgT* msg,
    ActionEagerSend<MsgT, BaseT> eager_sender = nullptr
  ) {
    auto eager_default_send = [=](SerializedEagerMsg<MsgT, BaseT>* m){
      theMsg()->sendMsg<SerialEagerPayloadMsg<MsgT, BaseT>,payloadMsgHandler>(
        dest, m
      );
    };
    auto eager = eager_sender ? eager_sender : eager_default_send;
    return sendSerialMsg<MsgT,f,BaseT>(msg, eager, [=](ActionNodeType action) {
      action(dest);
    });
  }

  template <typename MsgT, ActiveTypedFnType<MsgT> *f, typename BaseT = Message>
  static void broadcastSerialMsg(MsgT* msg) {
    using PayloadMsg = SerialEagerPayloadMsg<MsgT, BaseT>;

    HandlerType const& han = auto_registry::makeAutoHandler<MsgT, f>(nullptr);
    SerialEagerPayloadMsg<MsgT, BaseT>* payload_msg = nullptr;
    SizeType ptr_size = 0;
    SerialWrapperMsgType<MsgT>* sys_msg = nullptr;
    auto const& sys_size = sizeof(SerialWrapperMsgType<MsgT>);

    auto serialized_msg = serialize(
      *msg, [&](SizeType size) -> SerialByteType* {
        ptr_size = size;
        if (size > serialized_msg_eager_size) {
          char* buf = static_cast<char*>(thePool()->alloc(ptr_size + sys_size));
          sys_msg = new (buf) SerialWrapperMsgType<MsgT>{};
          return buf + sys_size;
        } else {
          payload_msg = makeSharedMessage<PayloadMsg>(size);
          return payload_msg->payload.data();
        }
      }
    );

    if (ptr_size < serialized_msg_eager_size) {
      assert(
        ptr_size < serialized_msg_eager_size &&
        "Must be smaller for eager protocol"
      );

      // move serialized msg envelope to system envelope to preserve info
      payload_msg->env = msg->env;
      payload_msg->handler = han;
      payload_msg->from_node = theContext()->getNode();

      debug_print(
        serial_msg, node,
        "broadcastSerialMsg: han={}, size={}, serialized_msg_eager_size={}\n",
        han, ptr_size, serialized_msg_eager_size
      );

      theMsg()->broadcastMsg<PayloadMsg,payloadMsgHandler>(payload_msg);
    } else {
      auto const& total_size = ptr_size + sys_size;

      sys_msg->handler = han;
      sys_msg->from_node = theContext()->getNode();
      sys_msg->ptr_size = ptr_size;

      debug_print(
        serial_msg, node,
        "broadcastSerialMsg: container: han={}, sys_size={}, ptr_size={}, "
        "total_size={}\n",
        han, sys_size, ptr_size, total_size
      );

      theMsg()->broadcastMsgSz<SerialWrapperMsgType<MsgT>,serialMsgHandlerBcast>(
        sys_msg, total_size, no_tag, no_action
      );
    }
  }

  template <typename MsgT, ActiveTypedFnType<MsgT> *f, typename BaseT = Message>
  static void sendSerialMsg(
    MsgT* msg, ActionEagerSend<MsgT, BaseT> eager_sender,
    ActionDataSend data_sender
  ) {
    HandlerType const& typed_handler =
      auto_registry::makeAutoHandler<MsgT, f>(nullptr);

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
            ptr_size
          );
          return payload_msg->payload.data();
        }
      }
    );

    //fmt::print("ptr_size={}\n", ptr_size);

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
          auto active_fn = auto_registry::getAutoHandler(typed_handler);
          active_fn(reinterpret_cast<BaseMessage*>(tptr));
          messageDeref(msg);
          //std::free(msg);
        }
      };

      data_sender(send_data);
    } else {
      assert(payload_msg != nullptr and eager_sender != nullptr);

      // move serialized msg envelope to system envelope to preserve info
      payload_msg->env = msg->env;
      payload_msg->handler = typed_handler;
      payload_msg->from_node = theContext()->getNode();

      eager_sender(payload_msg);
    }
  }
};

}} /* end namespace vt::serialization */

namespace vt {

using SerializedMessenger = ::vt::serialization::SerializedMessenger;

} /* end namespace vt */

#endif /*INCLUDED_SERIALIZATION_MESSAGING/SERIALIZED_MESSENGER_H*/
