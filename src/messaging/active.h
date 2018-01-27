
#if !defined INCLUDED_MESSAGING_ACTIVE_H
#define INCLUDED_MESSAGING_ACTIVE_H

#include <cstdint>
#include <memory>
#include <mpi.h>

#include "config.h"
#include "activefn/activefn.h"
#include "messaging/active.fwd.h"
#include "messaging/interface.h"
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

struct ActiveMessenger :
    messaging::interface::ActivePayloadFn,
    messaging::interface::ActiveCallbackFn,
    messaging::interface::ActivePreregisterFn,
    messaging::interface::ActiveFunctorFn,
    messaging::interface::ActiveBasicFn,
    messaging::interface::ActiveAutoFn
{
  using BufferedMsgType = BufferedActiveMsg;
  using MessageType = ShortMessage*;
  using CountType = int32_t;
  using PendingRecvType = PendingRecv;
  using EventRecordType = event::AsyncEvent::EventRecordType;
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

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  void trigger(std::function<void(vt::BaseMessage*)> fn);

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
