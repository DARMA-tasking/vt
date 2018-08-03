
#if !defined INCLUDED_SERIALIZATION_MESSAGING_SERIALIZED_MESSENGER_H
#define INCLUDED_SERIALIZATION_MESSAGING_SERIALIZED_MESSENGER_H

#include "config.h"
#include "messaging/message.h"
#include "serialization/serialization.h"
#include "serialization/messaging/serialized_data_msg.h"

#include <tuple>
#include <type_traits>
#include <cstdlib>
#include <functional>

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
  static void parserdesHandler(MsgT* msg);

  template <typename UserMsgT>
  static void serialMsgHandlerBcast(SerialWrapperMsgType<UserMsgT>* sys_msg);

  template <typename UserMsgT>
  static void serialMsgHandler(SerialWrapperMsgType<UserMsgT>* sys_msg);

  template <typename UserMsgT, typename BaseEagerMsgT>
  static void payloadMsgHandler(
    SerialEagerPayloadMsg<UserMsgT, BaseEagerMsgT>* sys_msg
  );

  template <typename MsgT, ActiveTypedFnType<MsgT> *f, typename BaseT = Message>
  static void sendParserdesMsg(NodeType dest, MsgT* msg);

  template <typename FunctorT, typename MsgT, typename BaseT = Message>
  static void sendParserdesMsg(NodeType dest, MsgT* msg);

  template <typename MsgT, typename BaseT = Message>
  static void sendParserdesMsgHandler(
    NodeType dest, HandlerType const& handler, MsgT* msg
  );

  template <typename FunctorT, typename MsgT, typename BaseT = Message>
  static void broadcastParserdesMsg(MsgT* msg);

  template <typename MsgT, ActiveTypedFnType<MsgT> *f, typename BaseT = Message>
  static void broadcastParserdesMsg(MsgT* msg);

  template <typename MsgT, typename BaseT = Message>
  static void broadcastParserdesMsgHandler(MsgT* msg, HandlerType const& han);

  template <typename MsgT, ActiveTypedFnType<MsgT> *f, typename BaseT = Message>
  static void parserdesMsg(
    MsgT* msg, bool is_bcast = true, NodeType dest = uninitialized_destination
  );

  template <typename FunctorT, typename MsgT, typename BaseT = Message>
  static void parserdesMsg(
    MsgT* msg, bool is_bcast = true, NodeType dest = uninitialized_destination
  );

  template <typename MsgT, typename BaseT = Message>
  static void parserdesMsgHandler(
    MsgT* msg, HandlerType const& handler, bool is_bcast = true,
    NodeType dest = uninitialized_destination
  );

  template <typename FunctorT, typename MsgT, typename BaseT = Message>
  static void sendSerialMsg(
    NodeType dest, MsgT* msg, ActionEagerSend<MsgT, BaseT> eager = nullptr
  );

  template <typename MsgT, ActiveTypedFnType<MsgT> *f, typename BaseT = Message>
  static void sendSerialMsg(
    NodeType dest, MsgT* msg, ActionEagerSend<MsgT, BaseT> eager = nullptr
  );

  template <typename MsgT, typename BaseT = Message>
  static void sendSerialMsgHandler(
    NodeType dest, MsgT* msg, HandlerType const& handler,
    ActionEagerSend<MsgT, BaseT> eager_sender = nullptr
  );

  template <typename FunctorT, typename MsgT, typename BaseT = Message>
  static void broadcastSerialMsg(MsgT* msg);

  template <typename MsgT, ActiveTypedFnType<MsgT> *f, typename BaseT = Message>
  static void broadcastSerialMsg(MsgT* msg);

  template <typename MsgT, typename BaseT = Message>
  static void broadcastSerialMsgHandler(MsgT* msg, HandlerType const& han);

  template <typename MsgT, ActiveTypedFnType<MsgT> *f, typename BaseT = Message>
  static void sendSerialMsg(
    MsgT* msg, ActionEagerSend<MsgT, BaseT> eager, ActionDataSend sender
  );

  template <typename MsgT, typename BaseT = Message>
  static void sendSerialMsgHandler(
    MsgT* msg, ActionEagerSend<MsgT, BaseT> eager_sender,
    HandlerType const& typed_handler, ActionDataSend data_sender
  );
};

}} /* end namespace vt::serialization */

namespace vt {

using SerializedMessenger = ::vt::serialization::SerializedMessenger;

} /* end namespace vt */

#include "serialization/messaging/serialized_messenger.impl.h"

#endif /*INCLUDED_SERIALIZATION_MESSAGING/SERIALIZED_MESSENGER_H*/
