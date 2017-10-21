
#if !defined INCLUDED_MESSAGING_ACTIVE_H
#define INCLUDED_MESSAGING_ACTIVE_H

#include <cstdint>
#include <memory>
#include <mpi.h>

#include "config.h"
#include "activefn/activefn.h"
#include "event/event.h"
#include "registry/registry.h"
#include "registry/auto_registry_interface.h"

#include <type_traits>
#include <tuple>
#include <vector>
#include <unordered_map>

namespace vt {

using MPI_TagType = int;

enum class MPITag : MPI_TagType {
  ActiveMsgTag = 1,
  DataMsgTag = 2
};

static constexpr TagType const starting_direct_buffer_tag = 1000;

struct PendingRecv {
  void* user_buf = nullptr;
  RDMA_ContinuationDeleteType cont = nullptr;
  ActionType dealloc_user_buf = nullptr;
  NodeType recv_node = uninitialized_destination;

  PendingRecv(
    void* in_user_buf, RDMA_ContinuationDeleteType in_cont,
    ActionType in_dealloc_user_buf, NodeType node
  ) : user_buf(in_user_buf), cont(in_cont), dealloc_user_buf(in_dealloc_user_buf),
      recv_node(node)
  { }
};

struct BufferedActiveMsg {
  using MessageType = ShortMessage*;

  MessageType buffered_msg;
  NodeType from_node;

  BufferedActiveMsg(
    MessageType const& in_buffered_msg, NodeType const& in_from_node
  ) : buffered_msg(in_buffered_msg), from_node(in_from_node)
  { }
};

struct ActiveMessenger {
  using BufferedMsgType = BufferedActiveMsg;
  using MessageType = ShortMessage*;
  using CountType = int32_t;
  using PendingRecvType = PendingRecv;
  using SendDataRetType = std::tuple<EventType, TagType>;
  using SendFnType = std::function<SendDataRetType(RDMA_GetType,NodeType,TagType,ActionType)>;
  using UserSendFnType = std::function<void(SendFnType)>;
  using ContainerPendingType = std::unordered_map<TagType, PendingRecvType>;
  using MsgContType = std::list<BufferedMsgType>;
  using ContainerWaitingHandlerType = std::unordered_map<HandlerType, MsgContType>;
  using ReadyHanTagType = std::tuple<HandlerType, TagType>;
  using MaybeReadyType = std::vector<ReadyHanTagType>;
  using HandlerManagerType = HandlerManager;

  ActiveMessenger() = default;

  template <typename MessageT>
  void setTermMessage(MessageT* const msg) {
    setTermType(msg->env);
  }

  template <typename MessageT>
  void setEpochMessage(MessageT* const msg, EpochType const& epoch) {
    envelopeSetEpoch(msg->env, epoch);
  }

  template <typename MessageT>
  void setTagMessage(MessageT* const msg, TagType const& tag) {
    envelopeSetTag(msg->env, tag);
  }

  /*----------------------------------------------------------------------------
   *            Basic Active Message Send with Pre-Registered Handler
   *----------------------------------------------------------------------------
   *
   * Send message  to pre-registered active message handler.
   *
   *   void myHandler(MyMsg* msg) {
   *     // do work ...
   *   }
   *
   *   HandlerType const han = registerNewHandler(my_handler);
   *
   *   MyMsg* msg = makeSharedMessage<MyMsg>(156);
   *   theMsg->sendMsg(29, han, msg);
   *
   *----------------------------------------------------------------------------
   */

  template <typename MessageT>
  EventType sendMsg(
    NodeType const& dest, HandlerType const& han, MessageT* const msg,
    ActionType next_action = nullptr
  ) {
    envelopeSetup(msg->env, dest, han);
    return sendDataDirect(han, msg, sizeof(MessageT), next_action);
  }

  template <typename MessageT>
  EventType sendMsg(
    HandlerType const& han, MessageT* const msg, TagType const& tag = no_tag,
    ActionType next_action = nullptr
  ) {
    auto const& dest = HandlerManagerType::getHandlerNode(han);
    assert(
      dest != uninitialized_destination and
      "Destination must be known in handler"
    );
    envelopeSetup(msg->env, dest, han);
    if (tag != no_tag) {
      envelopeSetTag(msg->env, tag);
    }
    return sendDataDirect(han, msg, sizeof(MessageT), next_action);
  }

  /*
   *----------------------------------------------------------------------------
   *          End Basic Active Message Send with Pre-Registered Handler
   *----------------------------------------------------------------------------
   */

  /*----------------------------------------------------------------------------
   *              Send Message Active Function (type-safe handler)
   *----------------------------------------------------------------------------
   *
   * Send message using a type-safe function handler. This is the predominant
   * way that the messenger is expected to be used.
   *
   *   void myHandler(MyMsg* msg) {
   *     // do work ...
   *   }
   *
   *  theMsg->sendMsg<MyMsg, myHandler>(1, msg);
   *
   *----------------------------------------------------------------------------
   */

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  EventType broadcastMsg(
    MessageT* const msg, TagType const& tag = no_tag, ActionType next_action = nullptr
  ) {
    HandlerType const& han = auto_registry::makeAutoHandler<MessageT,f>(msg);
    auto const& this_node = theContext->getNode();
    setBroadcastType(msg->env);
    if (tag != no_tag) {
      envelopeSetTag(msg->env, tag);
    }
    return sendMsg(this_node, han, msg, next_action);
  }

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  EventType broadcastMsg(MessageT* const msg, ActionType act) {
    return broadcastMsg<MessageT,f>(msg,no_tag,act);
  }

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  EventType sendMsg(
    NodeType const& dest, MessageT* const msg, TagType const& tag = no_tag,
    ActionType next_action = nullptr
  ) {
    HandlerType const& han = auto_registry::makeAutoHandler<MessageT,f>(msg);
    envelopeSetup(msg->env, dest, han);
    if (tag != no_tag) {
      envelopeSetTag(msg->env, tag);
    }
    return sendDataDirect(han, msg, sizeof(MessageT), next_action);
  }

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  EventType sendMsg(NodeType const& dest, MessageT* const msg, ActionType act) {
    return sendMsg<MessageT,f>(dest,msg,no_tag,act);
  }

  /*
   *----------------------------------------------------------------------------
   *             End Send Message Active Function (type-safe handler)
   *----------------------------------------------------------------------------
   */

  /*----------------------------------------------------------------------------
   *                 Send Message BASIC Active Function (deprecated?)
   *----------------------------------------------------------------------------
   *
   * Send message using basic function handler. These handlers are NOT type-safe
   * and require the user to cast their message to the correct type as so:
   *
   *   void basicHandler(vt::BaseMessage* msg_in) {
   *     MyMsg* msg = static_cast<MyMsg*>(msg_in);
   *     ...
   *   }
   *
   *  theMsg->sendMsg<basicHandler, MyMsg>(1, msg);
   *
   * Most likely this will be deprecated unless there is a use for this, since
   * type safety does not cost anything in terms of overhead (either at runtime
   * or compile-time).
   *
   *----------------------------------------------------------------------------
   */

  template <ActiveFnType* f, typename MessageT>
  EventType broadcastMsg(
    MessageT* const msg, TagType const& tag = no_tag, ActionType next_action = nullptr
  ) {
    HandlerType const& han = auto_registry::makeAutoHandler<MessageT,f>(msg);
    auto const& this_node = theContext->getNode();
    setBroadcastType(msg->env);
    if (tag != no_tag) {
      envelopeSetTag(msg->env, tag);
    }
    return sendMsg(this_node, han, msg, next_action);
  }

  template <ActiveFnType* f, typename MessageT>
  EventType broadcastMsg(MessageT* const msg, ActionType act) {
    return broadcastMsg<f,MessageT>(msg,no_tag,act);
  }

  template <ActiveFnType* f, typename MessageT>
  EventType sendMsg(
    NodeType const& dest, MessageT* const msg, TagType const& tag = no_tag,
    ActionType next_action = nullptr
  ) {
    HandlerType const& han = auto_registry::makeAutoHandler<MessageT,f>(msg);
    envelopeSetup(msg->env, dest, han);
    if (tag != no_tag) {
      envelopeSetTag(msg->env, tag);
    }
    return sendDataDirect(han, msg, sizeof(MessageT), next_action);
  }

  template <ActiveFnType* f, typename MessageT>
  EventType sendMsg(NodeType const& dest, MessageT* const msg, ActionType act) {
    return sendMsg<f,MessageT>(dest,msg,no_tag,act);
  }

  /*
   *----------------------------------------------------------------------------
   *              End Send Message BASIC Active Function (deprecated?)
   *----------------------------------------------------------------------------
   */

  /*----------------------------------------------------------------------------
   *                       Send Message Functor Variants
   *----------------------------------------------------------------------------
   *
   * Send message Functor variants that cause an active message to trigger a
   * user-defined functor such as:
   *
   *   struct X {
   *     void operator()(MyMsg* msg) const { ... };
   *   };
   *
   *----------------------------------------------------------------------------
   */

  template <typename FunctorT, typename MessageT>
  EventType broadcastMsg(
    MessageT* const msg, TagType const& tag = no_tag, ActionType next_action = nullptr
  ) {
    HandlerType const& han =
      auto_registry::makeAutoHandlerFunctor<FunctorT, true, MessageT*>();
    setBroadcastType(msg->env);
    if (tag != no_tag) {
      envelopeSetTag(msg->env, tag);
    }
    return sendMsg(theContext->getNode(), han, msg, next_action);
  }

  template <typename FunctorT, typename MessageT>
  EventType broadcastMsg(MessageT* const msg, ActionType act) {
    return broadcastMsg<FunctorT,MessageT>(msg,no_tag,act);
  }

  template <typename FunctorT, typename MessageT>
  EventType sendMsg(
    NodeType const& dest, MessageT* const msg, TagType const& tag = no_tag,
    ActionType next_action = nullptr
  ) {
    HandlerType const& han =
      auto_registry::makeAutoHandlerFunctor<FunctorT, true, MessageT*>();
    envelopeSetup(msg->env, dest, han);
    if (tag != no_tag) {
      envelopeSetTag(msg->env, tag);
    }
    return sendDataDirect(han, msg, sizeof(MessageT), next_action);
  }

  template <typename FunctorT, typename MessageT>
  EventType sendMsg(NodeType const& dest, MessageT* const msg, ActionType act) {
    return sendMsg<FunctorT,MessageT>(dest,msg,no_tag,act);
  }

  /*
   *----------------------------------------------------------------------------
   *                      End Send Message Functor Variants
   *----------------------------------------------------------------------------
   */

  /*----------------------------------------------------------------------------
   *                     Send Data Message (includes payload)
   *----------------------------------------------------------------------------
   *
   * Send message that includes a payload that can be arbitrary data that is
   * coordinated by the system
   *
   *----------------------------------------------------------------------------
   */
  template <typename MessageT>
  EventType sendMsg(
    NodeType const& dest, HandlerType const& han, MessageT* const msg,
    UserSendFnType send_payload_fn, ActionType next_action = nullptr
  ) {
    using namespace std::placeholders;

    // must send first so action payload function runs before the send
    auto f = std::bind(&ActiveMessenger::sendData, this, _1, _2, _3, _4);
    send_payload_fn(f);

    // setup envelope
    envelopeSetup(msg->env, dest, han);
    auto const& ret = sendDataDirect(han, msg, sizeof(MessageT), next_action);

    return ret;
  }

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  EventType sendMsg(
    NodeType const& dest, MessageT* const msg, UserSendFnType send_payload_fn,
    ActionType next_action = nullptr
  ) {
    HandlerType const& han = auto_registry::makeAutoHandler<MessageT,f>(msg);
    return sendMsg<MessageT>(dest, han, msg, send_payload_fn, next_action);
  }

  SendDataRetType sendData(
    RDMA_GetType const& ptr, NodeType const& dest, TagType const& tag,
    ActionType next_action = nullptr
  );

  bool recvDataMsg(
    TagType const& tag, NodeType const& node, RDMA_ContinuationDeleteType next = nullptr
  );

  bool recvDataMsg(
    TagType const& tag, NodeType const& recv_node, bool const& enqueue,
    RDMA_ContinuationDeleteType next = nullptr
  );

  bool recvDataMsgBuffer(
    void* const user_buf, TagType const& tag,
    NodeType const& node = uninitialized_destination, bool const& enqueue = true,
    ActionType dealloc_user_buf = nullptr, RDMA_ContinuationDeleteType next = nullptr
  );

  template <typename MessageT>
  EventType broadcastMsg(
    HandlerType const& han, MessageT* const msg, ActionType next_action = nullptr
  ) {
    auto const& this_node = theContext->getNode();
    setBroadcastType(msg->env);
    return sendMsg(this_node, han, msg, next_action);
  }

  EventType sendDataDirect(
    HandlerType const& han, BaseMessage* const msg, int const& msg_size,
    ActionType next_action = nullptr
  );

  /*
   *----------------------------------------------------------------------------
   *                           End Send Data Message
   *----------------------------------------------------------------------------
   */

   /*----------------------------------------------------------------------------
   *                            Send Message Callback
   *----------------------------------------------------------------------------
   *
   * Send message *callback* variants (automatically allow user to callback upon
   * message arrival)
   *
   *----------------------------------------------------------------------------
   */
  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  EventType sendDataCallback(
    NodeType const& dest, MessageT* const msg, ActiveClosureFnType fn
  ) {
    HandlerType const& this_han = registerNewHandler(fn, no_tag);
    auto cb = static_cast<CallbackMessage*>(msg);
    cb->setCallback(this_han);
    return sendMsg<MessageT,f>(dest, msg, no_tag, nullptr);
  }

  template <typename MessageT>
  void sendDataCallback(
    HandlerType const& han, NodeType const& dest, MessageT* const msg,
    ActiveClosureFnType fn
  ) {
    HandlerType const& this_han = registerNewHandler(fn, no_tag);
    auto cb = static_cast<CallbackMessage*>(msg);
    cb->setCallback(this_han);
    sendMsg(dest, han, msg);
  }

  template <typename MessageT>
  void sendCallback(MessageT* const msg) {
    auto const& han_callback = getCurrentCallback();
    sendMsg(han_callback, msg);
  }

  /*
   *----------------------------------------------------------------------------
   *                        End Send Message Callback
   *----------------------------------------------------------------------------
   */

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  void trigger(std::function<void(vt::BaseMessage*)> fn) {
    HandlerType const& han = auto_registry::makeAutoHandler<MessageT,f>(nullptr);
    theRegistry->saveTrigger(han, /*reinterpret_cast<active_function_t>(*/fn);
  }

  void performTriggeredActions();
  bool tryProcessIncomingMessage();
  bool processDataMsgRecv();
  bool scheduler();
  bool isLocalTerm();

  HandlerType registerNewHandler(ActiveClosureFnType fn, TagType const& tag = no_tag);
  void swapHandlerFn(
    HandlerType const& han, ActiveClosureFnType fn, TagType const& tag = no_tag
  );
  void unregisterHandlerFn(HandlerType const& han, TagType const& tag = no_tag);
  void registerHandlerFn(
    HandlerType const& han, ActiveClosureFnType fn, TagType const& tag = no_tag
  );
  HandlerType collectiveRegisterHandler(ActiveClosureFnType fn, TagType const& tag = no_tag);

  HandlerType getCurrentHandler();
  HandlerType getCurrentCallback();
  NodeType getFromNodeCurrentHandler();

  bool deliverActiveMsg(MessageType msg, NodeType const& from_node, bool insert);
  void deliverPendingMsgsHandler(HandlerType const& han, TagType const& tag = no_tag);
  void processMaybeReadyHanTag();

private:
  HandlerType current_handler_context_ = uninitialized_handler;
  HandlerType current_callback_context_ = uninitialized_handler;
  NodeType current_node_context_ = uninitialized_destination;

  MaybeReadyType maybe_ready_tag_han_;
  ContainerWaitingHandlerType pending_handler_msgs_;
  ContainerPendingType pending_recvs_;
  TagType cur_direct_buffer_tag_ = starting_direct_buffer_tag;
};

extern std::unique_ptr<ActiveMessenger> theMsg;

} //end namespace vt

#endif /*INCLUDED_MESSAGING_ACTIVE_H*/
