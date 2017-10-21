
#if !defined INCLUDED_SERIALIZATION_MESSAGING_SERIALIZED_PARAM_MESSENGER_H
#define INCLUDED_SERIALIZATION_MESSAGING_SERIALIZED_PARAM_MESSENGER_H

#include "config.h"
#include "messaging/message.h"
#include "messaging/active.h"
#include "registry/auto_registry_interface.h"
#include "registry/auto_registry_vc.h"
#include "serialization/serialization.h"
#include "serialization/messaging/serialized_data_msg.h"
#include "parameterization/param_meta.h"

#include <tuple>
#include <utility>

namespace vt { namespace serialization {

using namespace ::vt::param;

#define SERIAL_MSG_HAN(value) PARAM_FUNCTION(value)

template<typename T, T value>
using SerializedNonType = ::vt::param::NonType<T, value>;

struct SerializedMessengerParam {
  template <typename T>
  using MsgType = SerializedDataMsg<T>;

  template <typename Tuple>
  static void serializedMsgHandler(MsgType<Tuple>* msg) {
    auto fn = auto_registry::getAutoHandler(msg->handler);
    auto const& recv_tag = msg->data_recv_tag;
    theMsg->recvDataMsg(
      recv_tag, msg->from_node, [=](RDMA_GetType ptr, ActionType){
        auto raw_ptr = reinterpret_cast<SerialByteType*>(std::get<0>(ptr));
        auto ptr_size = std::get<1>(ptr);
        auto& t1 = deserialize<Tuple>(raw_ptr, ptr_size, nullptr);
        invokeCallableTuple(std::forward<Tuple>(t1), fn, false);
      }
    );
  }

  template <typename T, T value, typename... Args>
  static void sendSerdesParamMsg(
    NodeType const& dest, std::tuple<Args...> tup,
    SerializedNonType<T, value> __attribute__((unused)) non = SerializedNonType<T,value>()
  ) {
    auto const& typed_handler = auto_registry::makeAutoHandler<T,value>();

    using TupleType = std::tuple<Args...>;

    auto meta_typed_data_msg = new SerializedDataMsg<TupleType>();

    SerialByteType* ptr = nullptr;
    SizeType ptr_size = 0;

    auto serialized = serialize(tup, [&](SizeType size) -> SerialByteType*{
      ptr_size = size;
      ptr = static_cast<SerialByteType*>(malloc(size));
      return ptr;
    });

    auto send_serialized = [&](ActiveMessenger::SendFnType send){
      auto ret = send(RDMA_GetType{ptr, ptr_size}, dest, no_tag, no_action);
      meta_typed_data_msg->data_recv_tag = std::get<1>(ret);
    };

    meta_typed_data_msg->handler = typed_handler;
    meta_typed_data_msg->from_node = theContext->getNode();
    setPutType(meta_typed_data_msg->env);

    auto deleter = [=]{ delete meta_typed_data_msg; };

    theMsg->sendMsg<MsgType<TupleType>, serializedMsgHandler>(
      dest, meta_typed_data_msg, send_serialized, deleter
    );
  }

};


}} /* end namespace vt::serialization */

#endif /*INCLUDED_SERIALIZATION_MESSAGING/SERIALIZED_PARAM_MESSENGER_H*/
