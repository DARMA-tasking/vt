
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

  template <typename Vrt, typename Tuple, typename UserMsgT>
  static void virtualMsgHandler(VrtMsgType<Vrt, Tuple, UserMsgT>* msg) {
    auto vc_active_fn = auto_registry::getAutoHandlerVC(msg->handler);
    auto const& recv_tag = msg->data_recv_tag;
    theMsg->recvDataMsg(
      recv_tag, msg->from_node, [=](RDMA_GetType ptr, ActionType){
        auto raw_ptr = reinterpret_cast<SerialByteType*>(std::get<0>(ptr));
        auto ptr_size = std::get<1>(ptr);
        auto tptr = deserialize<Tuple>(raw_ptr, ptr_size, nullptr);
        UserMsgT msg(tptr);
        vc_active_fn(&msg, nullptr);
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

    theMsg->sendMsg<VrtMsgType<VcT, TupleType, MsgT>, virtualMsgHandler>(
      dest, meta_typed_data_msg, send_serialized, deleter
    );
  }
};

}} /* end namespace vt::serialization */

namespace vt {

using SerializedMessenger = ::vt::serialization::SerializedMessenger;

} /* end namespace vt */

#endif /*__RUNTIME_TRANSPORT_SERIALIZED_MSG__*/
