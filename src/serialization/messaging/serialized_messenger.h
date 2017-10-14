
#if !defined __RUNTIME_TRANSPORT_SERIALIZED_MSG__
#define __RUNTIME_TRANSPORT_SERIALIZED_MSG__

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

struct SerializedMessenger {
  template <typename Vrt, typename T, typename UserMsgT>
  using VrtMsgType = SerializedDataMsg<T, UserMsgT, Vrt>;

  template <typename T, typename UserMsgT>
  using MsgType = SerializedDataMsg<T, UserMsgT>;

  template <typename Vrt, typename Tuple, typename UserMsgT>
  static void serialMsgHandler(VrtMsgType<Vrt, Tuple, UserMsgT>* msg) {
    auto const handler = msg->handler;
    auto const is_virtual = msg->is_virtual;
    auto const& recv_tag = msg->data_recv_tag;
    theMsg->recvDataMsg(
      recv_tag, msg->from_node, [handler,is_virtual](RDMA_GetType ptr, ActionType){
        // be careful here not to use "msg", it is no longer valid
        auto raw_ptr = reinterpret_cast<SerialByteType*>(std::get<0>(ptr));
        auto ptr_size = std::get<1>(ptr);
        auto tptr = deserialize<Tuple>(raw_ptr, ptr_size, nullptr);
        UserMsgT user_msg(tptr);

        if (is_virtual) {
          auto vc_active_fn = auto_registry::getAutoHandlerVC(handler);
          vc_active_fn(&user_msg, nullptr);
        } else {
          auto active_fn = auto_registry::getAutoHandler(handler);
          active_fn(&user_msg);
        }
      }
    );
  }

  template <
    typename VcT,
    typename MsgT,
    auto_registry::ActiveVrtTypedFnType<MsgT, VcT> *f,
    typename... Args
  >
  static void sendSerialVirualMsg(NodeType const& dest, std::tuple<Args...>&& tup) {
    HandlerType const& typed_handler =
      auto_registry::makeAutoHandlerVC<VcT, MsgT, f>(nullptr);

    using TupleType = typename std::decay<decltype(tup)>::type;
    auto meta_typed_data_msg = new VrtMsgType<VcT, TupleType, MsgT>();
    meta_typed_data_msg->is_virtual = true;

    SerialByteType* ptr = nullptr;
    SizeType ptr_size = 0;

    auto serialized = serialize(
      tup, [&](SizeType size) -> SerialByteType* {
        ptr_size = size;
        ptr = static_cast<SerialByteType*>(malloc(size));
        return ptr;
      }
    );

    auto send_serialized = [&](ActiveMessenger::SendFnType send){
      auto ret = send(RDMA_GetType{ptr, ptr_size}, dest, no_tag, no_action);
      meta_typed_data_msg->data_recv_tag = std::get<1>(ret);
    };

    meta_typed_data_msg->handler = typed_handler;
    meta_typed_data_msg->from_node = theContext->getNode();

    setPutType(meta_typed_data_msg->env);

    auto deleter = [=]{ delete meta_typed_data_msg; };

    theMsg->sendMsg<VrtMsgType<VcT, TupleType, MsgT>, serialMsgHandler>(
      dest, meta_typed_data_msg, send_serialized, deleter
    );
  }

  template <typename MsgT, ActiveTypedFnType<MsgT> *f, typename... Args>
  static void sendSerialMsg(NodeType const& dest, std::tuple<Args...>&& tup) {
    HandlerType const& typed_handler =
      auto_registry::makeAutoHandler<MsgT, f>(nullptr);

    using TupleType = typename std::decay<decltype(tup)>::type;
    auto meta_typed_data_msg = new MsgType<TupleType, MsgT>();
    meta_typed_data_msg->is_virtual = false;

    SerialByteType* ptr = nullptr;
    SizeType ptr_size = 0;

    auto serialized = serialize(
      tup, [&](SizeType size) -> SerialByteType* {
        ptr_size = size;
        ptr = static_cast<SerialByteType*>(malloc(size));
        return ptr;
      }
    );

    auto send_serialized = [&](ActiveMessenger::SendFnType send){
      auto ret = send(RDMA_GetType{ptr, ptr_size}, dest, no_tag, no_action);
      meta_typed_data_msg->data_recv_tag = std::get<1>(ret);
    };

    meta_typed_data_msg->handler = typed_handler;
    meta_typed_data_msg->from_node = theContext->getNode();

    setPutType(meta_typed_data_msg->env);

    auto deleter = [=]{ delete meta_typed_data_msg; };

    theMsg->sendMsg<MsgType<TupleType, MsgT>, serialMsgHandler>(
      dest, meta_typed_data_msg, send_serialized, deleter
    );
  }
};

}} /* end namespace vt::serialization */

namespace vt {

using SerializedMessenger = ::vt::serialization::SerializedMessenger;

} /* end namespace vt */

#endif /*__RUNTIME_TRANSPORT_SERIALIZED_MSG__*/
