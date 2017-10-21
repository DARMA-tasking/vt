
#if !defined INCLUDED_SERIALIZATION_MESSAGING_SERIALIZED_MESSENGER_H
#define INCLUDED_SERIALIZATION_MESSAGING_SERIALIZED_MESSENGER_H

#include "config.h"
#include "messaging/message.h"
#include "messaging/active.h"
#include "registry/auto_registry_interface.h"
#include "registry/auto_registry_vc.h"
#include "serialization/serialization.h"
#include "serialization/messaging/serialized_data_msg.h"

#include <tuple>
#include <type_traits>

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

  template <typename UserMsgT>
  static void serialMsgHandler(SerialWrapperMsgType<UserMsgT>* sys_msg) {
    auto const handler = sys_msg->handler;
    auto const& recv_tag = sys_msg->data_recv_tag;
    theMsg->recvDataMsg(
      recv_tag, sys_msg->from_node, [handler](RDMA_GetType ptr, ActionType){
        // be careful here not to use "msg", it is no longer valid
        auto raw_ptr = reinterpret_cast<SerialByteType*>(std::get<0>(ptr));
        auto ptr_size = std::get<1>(ptr);
        UserMsgT* msg = new UserMsgT;
        auto tptr = deserialize<UserMsgT>(raw_ptr, ptr_size, msg);

        auto active_fn = auto_registry::getAutoHandler(handler);
        active_fn(reinterpret_cast<BaseMessage*>(tptr));
      }
    );
  }

  template <typename UserMsgT, typename BaseEagerMsgT>
  static void payloadMsgHandler(
    SerialEagerPayloadMsg<UserMsgT, BaseEagerMsgT>* sys_msg
  ) {
    auto const handler = sys_msg->handler;

    UserMsgT* user_msg = new UserMsgT;
    auto tptr = deserialize<UserMsgT>(
      sys_msg->payload.data(), sys_msg->bytes, user_msg
    );

    auto active_fn = auto_registry::getAutoHandler(handler);
    active_fn(reinterpret_cast<BaseMessage*>(tptr));
  }

  template <typename MsgT, ActiveTypedFnType<MsgT> *f, typename BaseT = Message>
  static void sendSerialMsg(
    NodeType dest, MsgT* msg,
    ActionEagerSend<MsgT, BaseT> eager_sender = nullptr
  ) {
    auto eager_default_send = [=](SerializedEagerMsg<MsgT, BaseT>* m){
      theMsg->sendMsg<SerialEagerPayloadMsg<MsgT, BaseT>,payloadMsgHandler>(
        dest, m
      );
    };
    auto eager = eager_sender ? eager_sender : eager_default_send;
    return sendSerialMsg<MsgT,f,BaseT>(msg, eager, [=](ActionNodeType action) {
      action(dest);
    });
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

    //printf("ptr_size=%ld\n", ptr_size);

    if (ptr_size > serialized_msg_eager_size) {
      auto sys_msg = makeSharedMessage<SerialWrapperMsgType<MsgT>>();
      // move serialized msg envelope to system envelope to preserve info
      //sys_msg->env = msg->env;

      assert(payload_msg == nullptr and data_sender != nullptr);

      auto send_data = [=](NodeType dest){
        auto send_serialized = [&](ActiveMessenger::SendFnType send){
          auto ret = send(RDMA_GetType{ptr, ptr_size}, dest, no_tag, no_action);
          sys_msg->data_recv_tag = std::get<1>(ret);
        };

        sys_msg->handler = typed_handler;
        sys_msg->from_node = theContext->getNode();

        setPutType(sys_msg->env);

        theMsg->sendMsg<SerialWrapperMsgType<MsgT>, serialMsgHandler>(
          dest, sys_msg, send_serialized
        );
      };

      data_sender(send_data);
    } else {
      assert(payload_msg != nullptr and eager_sender != nullptr);

      // move serialized msg envelope to system envelope to preserve info
      payload_msg->env = msg->env;
      payload_msg->handler = typed_handler;
      payload_msg->from_node = theContext->getNode();

      eager_sender(payload_msg);
    }
  }
};

}} /* end namespace vt::serialization */

namespace vt {

using SerializedMessenger = ::vt::serialization::SerializedMessenger;

} /* end namespace vt */

#endif /*INCLUDED_SERIALIZATION_MESSAGING/SERIALIZED_MESSENGER_H*/
