
#if !defined INCLUDED_MESSAGING_ACTIVE_H
#define INCLUDED_MESSAGING_ACTIVE_H

#include <cstdint>
#include <memory>
#include <mpi.h>

#include "config.h"
#include "activefn/activefn.h"
#include "messaging/active.fwd.h"
#include "event/event.h"
#include "registry/registry.h"
#include "registry/auto_registry_interface.h"

#include <type_traits>
#include <tuple>
#include <vector>
#include <unordered_map>
#include <limits>

namespace vt {

using MPI_TagType = int;

static constexpr TagType const PutPackedTag =
  std::numeric_limits<TagType>::max();

enum class MPITag : MPI_TagType {
  ActiveMsgTag = 1,
  DataMsgTag = 2
};

static constexpr TagType const starting_direct_buffer_tag = 1000;
static constexpr MsgSizeType const max_pack_direct_size = 512;

struct PendingRecv {
  void* user_buf = nullptr;
  RDMA_ContinuationDeleteType cont = nullptr;
  ActionType dealloc_user_buf = nullptr;
  NodeType recv_node = uninitialized_destination;

  PendingRecv(
    void* in_user_buf, RDMA_ContinuationDeleteType in_cont,
    ActionType in_dealloc_user_buf, NodeType node
  ) : user_buf(in_user_buf), cont(in_cont),
      dealloc_user_buf(in_dealloc_user_buf), recv_node(node)
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
  using EventRecordType = event::AsyncEvent::EventRecordType;
  using SendDataRetType = std::tuple<EventType, TagType>;
  using SendFnType = std::function<
    SendDataRetType(RDMA_GetType,NodeType,TagType,ActionType)
  >;
  using UserSendFnType = std::function<void(SendFnType)>;
  using ContainerPendingType = std::unordered_map<TagType, PendingRecvType>;
  using MsgContType = std::list<BufferedMsgType>;
  using ContainerWaitingHandlerType = std::unordered_map<
    HandlerType, MsgContType
  >;
  using ReadyHanTagType = std::tuple<HandlerType, TagType>;
  using MaybeReadyType = std::vector<ReadyHanTagType>;
  using HandlerManagerType = HandlerManager;

  ActiveMessenger();

  template <typename MessageT>
  void setTermMessage(MessageT* const msg);

  template <typename MessageT>
  void setEpochMessage(MessageT* const msg, EpochType const& epoch);

  template <typename MessageT>
  void setTagMessage(MessageT* const msg, TagType const& tag);

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
   *   theMsg()->sendMsg(29, han, msg);
   *
   *----------------------------------------------------------------------------
   */

  template <typename MessageT>
  EventType sendMsg(
    NodeType const& dest, HandlerType const& han, MessageT* const msg,
    ActionType next_action = nullptr
  );

  template <typename MessageT>
  EventType sendMsg(
    HandlerType const& han, MessageT* const msg, TagType const& tag = no_tag,
    ActionType next_action = nullptr
  );

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
   *  theMsg()->sendMsg<MyMsg, myHandler>(1, msg);
   *
   *----------------------------------------------------------------------------
   */

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  EventType broadcastMsg(
    MessageT* const msg, TagType const& tag = no_tag,
    ActionType next_action = nullptr
  );

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  EventType broadcastMsg(MessageT* const msg, ActionType act);

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  EventType sendMsg(
    NodeType const& dest, MessageT* const msg, TagType const& tag = no_tag,
    ActionType next_action = nullptr
  );

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  EventType sendMsg(NodeType const& dest, MessageT* const msg, ActionType act);

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
   *  theMsg()->sendMsg<basicHandler, MyMsg>(1, msg);
   *
   * Most likely this will be deprecated unless there is a use for this, since
   * type safety does not cost anything in terms of overhead (either at runtime
   * or compile-time).
   *
   *----------------------------------------------------------------------------
   */

  template <ActiveFnType* f, typename MessageT>
  EventType broadcastMsg(
    MessageT* const msg, TagType const& tag = no_tag,
    ActionType next_action = nullptr
  );

  template <ActiveFnType* f, typename MessageT>
  EventType broadcastMsg(MessageT* const msg, ActionType act);

  template <ActiveFnType* f, typename MessageT>
  EventType sendMsg(
    NodeType const& dest, MessageT* const msg, TagType const& tag = no_tag,
    ActionType next_action = nullptr
  );

  template <ActiveFnType* f, typename MessageT>
  EventType sendMsg(NodeType const& dest, MessageT* const msg, ActionType act);

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
    MessageT* const msg, TagType const& tag = no_tag,
    ActionType next_action = nullptr
  );

  template <typename FunctorT, typename MessageT>
  EventType broadcastMsg(MessageT* const msg, ActionType act);

  template <typename FunctorT, typename MessageT>
  EventType sendMsg(
    NodeType const& dest, MessageT* const msg, TagType const& tag = no_tag,
    ActionType next_action = nullptr
  );

  template <typename FunctorT, typename MessageT>
  EventType sendMsg(NodeType const& dest, MessageT* const msg, ActionType act);

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
  );

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  EventType sendMsg(
    NodeType const& dest, MessageT* const msg, UserSendFnType send_payload_fn,
    ActionType next_action = nullptr
  );

  template <typename MessageT>
  EventType broadcastMsg(
    HandlerType const& han, MessageT* const msg, ActionType next_action = nullptr
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
  );

  template <typename MessageT>
  void sendDataCallback(
    HandlerType const& han, NodeType const& dest, MessageT* const msg,
    ActiveClosureFnType fn
  );

  template <typename MessageT>
  void sendCallback(MessageT* const msg);

  /*
   *----------------------------------------------------------------------------
   *                        End Send Message Callback
   *----------------------------------------------------------------------------
   */

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  void trigger(std::function<void(vt::BaseMessage*)> fn);

  void packMsg(
    MessageType const msg, MsgSizeType const& size, void* ptr,
    MsgSizeType const& ptr_bytes
  );

  SendDataRetType sendData(
    RDMA_GetType const& ptr, NodeType const& dest, TagType const& tag,
    ActionType next_action = nullptr
  );

  bool recvDataMsg(
    TagType const& tag, NodeType const& node,
    RDMA_ContinuationDeleteType next = nullptr
  );

  bool recvDataMsg(
    TagType const& tag, NodeType const& recv_node, bool const& enqueue,
    RDMA_ContinuationDeleteType next = nullptr
  );

  bool recvDataMsgBuffer(
    void* const user_buf, TagType const& tag,
    NodeType const& node = uninitialized_destination, bool const& enqueue = true,
    ActionType dealloc_user_buf = nullptr,
    RDMA_ContinuationDeleteType next = nullptr
  );

  EventType sendMsgSized(
    HandlerType const& han, BaseMessage* const msg, MsgSizeType const& msg_size,
    ActionType next_action = nullptr
  );

  void performTriggeredActions();
  bool tryProcessIncomingMessage();
  bool processDataMsgRecv();
  bool scheduler();
  bool isLocalTerm();

  HandlerType registerNewHandler(
    ActiveClosureFnType fn, TagType const& tag = no_tag
  );
  void swapHandlerFn(
    HandlerType const& han, ActiveClosureFnType fn, TagType const& tag = no_tag
  );
  void unregisterHandlerFn(HandlerType const& han, TagType const& tag = no_tag);
  void registerHandlerFn(
    HandlerType const& han, ActiveClosureFnType fn, TagType const& tag = no_tag
  );
  HandlerType collectiveRegisterHandler(
    ActiveClosureFnType fn, TagType const& tag = no_tag
  );

  HandlerType getCurrentHandler();
  HandlerType getCurrentCallback();
  NodeType getFromNodeCurrentHandler();

  bool handleActiveMsg(
    MessageType msg, NodeType const& sender, MsgSizeType const& size, bool insert
  );
  bool deliverActiveMsg(MessageType msg, NodeType const& from_node, bool insert);
  void deliverPendingMsgsHandler(
    HandlerType const& han, TagType const& tag = no_tag
  );
  void processMaybeReadyHanTag();

  EventType sendMsgBytes(
    NodeType const& dest, BaseMessage* const base_msg,
    MsgSizeType const& msg_size, TagType const& send_tag,
    EventRecordType* parent_event, ActionType next_action
  );

  EventType sendMsgBytesWithPut(
    NodeType const& dest, BaseMessage* const base, MsgSizeType const& msg_size,
    TagType const& send_tag, EventRecordType* parent_event,
    ActionType next_action, EventType const& in_event
  );

private:
  NodeType this_node_ = uninitialized_destination;
  HandlerType current_handler_context_ = uninitialized_handler;
  HandlerType current_callback_context_ = uninitialized_handler;
  NodeType current_node_context_ = uninitialized_destination;

  MaybeReadyType maybe_ready_tag_han_;
  ContainerWaitingHandlerType pending_handler_msgs_;
  ContainerPendingType pending_recvs_;
  TagType cur_direct_buffer_tag_ = starting_direct_buffer_tag;
};

extern ActiveMessenger* theMsg();

} //end namespace vt

#include "messaging/active.impl.h"

#endif /*INCLUDED_MESSAGING_ACTIVE_H*/
