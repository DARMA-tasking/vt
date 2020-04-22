/*
//@HEADER
// *****************************************************************************
//
//                                   active.h
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_MESSAGING_ACTIVE_H
#define INCLUDED_MESSAGING_ACTIVE_H

#include <cstdint>
#include <memory>
#include <mpi.h>

#include "vt/config.h"
#include "vt/configs/arguments/args.h"
#include "vt/activefn/activefn.h"
#include "vt/messaging/active.fwd.h"
#include "vt/messaging/message/smart_ptr.h"
#include "vt/messaging/pending_send.h"
#include "vt/messaging/listener.h"
#include "vt/messaging/irecv_holder.h"
#include "vt/event/event.h"
#include "vt/registry/registry.h"
#include "vt/registry/auto/auto_registry_interface.h"
#include "vt/trace/trace_common.h"
#include "vt/utils/static_checks/functor.h"

#if backend_check_enabled(trace_enabled)
  #include "vt/trace/trace_headers.h"
#endif

#include <type_traits>
#include <tuple>
#include <vector>
#include <unordered_map>
#include <limits>
#include <stack>

namespace vt { namespace messaging {

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
  PriorityType priority = no_priority;

  PendingRecv(
    void* in_user_buf, RDMA_ContinuationDeleteType in_cont,
    ActionType in_dealloc_user_buf, NodeType node,
    PriorityType in_priority
  ) : user_buf(in_user_buf), cont(in_cont),
      dealloc_user_buf(in_dealloc_user_buf), recv_node(node),
      priority(in_priority)
  { }
};

struct InProgressIRecv {
  using CountType = int32_t;

  InProgressIRecv(
    char* in_buf, CountType in_probe_bytes, NodeType in_sender,
    MPI_Request in_req
  ) : buf(in_buf), probe_bytes(in_probe_bytes), sender(in_sender),
      req(in_req), valid(true)
  { }

  char* buf = nullptr;
  CountType probe_bytes = 0;
  NodeType sender = uninitialized_destination;
  MPI_Request req;
  bool valid = false;
};

struct InProgressDataIRecv : public InProgressIRecv {
  InProgressDataIRecv(
    char* in_buf, CountType in_probe_bytes, NodeType in_sender,
    MPI_Request in_req, void* const in_user_buf,
    ActionType in_dealloc_user_buf,
    RDMA_ContinuationDeleteType in_next,
    PriorityType in_priority
  ) : InProgressIRecv{in_buf, in_probe_bytes, in_sender, in_req},
      user_buf(in_user_buf), dealloc_user_buf(in_dealloc_user_buf),
      next(in_next), priority(in_priority)
  { }

  void* user_buf = nullptr;
  ActionType dealloc_user_buf = nullptr;
  RDMA_ContinuationDeleteType next = nullptr;
  PriorityType priority = no_priority;
};

struct BufferedActiveMsg {
  using MessageType = MsgSharedPtr<BaseMsgType>;

  MessageType buffered_msg;
  NodeType from_node;
  ActionType cont;

  BufferedActiveMsg(
    MessageType const& in_buffered_msg, NodeType const& in_from_node,
    ActionType in_cont
  ) : buffered_msg(in_buffered_msg), from_node(in_from_node), cont(in_cont)
  { }
};

struct ActiveMessenger {
  using BufferedMsgType      = BufferedActiveMsg;
  using MessageType          = ShortMessage*;
  using CountType            = int32_t;
  using PendingRecvType      = PendingRecv;
  using EventRecordType      = event::AsyncEvent::EventRecordType;
  using SendDataRetType      = std::tuple<EventType, TagType>;
  using SendFnType           = std::function<
    SendDataRetType(RDMA_GetType,NodeType,TagType)
  >;
  using UserSendFnType       = std::function<void(SendFnType)>;
  using ContainerPendingType = std::unordered_map<TagType,PendingRecvType>;
  using MsgContType          = std::list<BufferedMsgType>;
  using ContWaitType         = std::unordered_map<HandlerType, MsgContType>;
  using ReadyHanTagType      = std::tuple<HandlerType, TagType>;
  using MaybeReadyType       = std::vector<ReadyHanTagType>;
  using HandlerManagerType   = HandlerManager;
  using EpochStackType       = std::stack<EpochType>;
  using PendingSendType      = PendingSend;
  using ListenerType         = std::unique_ptr<Listener>;
  using ArgType              = vt::arguments::ArgConfig;

  ActiveMessenger();

  virtual ~ActiveMessenger();

  template <typename MsgPtrT>
  void setTermMessage(MsgPtrT const msg);

  template <typename MsgPtrT>
  void setEpochMessage(MsgPtrT const msg, EpochType const& epoch);

  template <typename MsgPtrT>
  void setTagMessage(MsgPtrT const msg, TagType const& tag);

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
  PendingSendType sendMsgSz(
    NodeType const& dest, HandlerType const& han, MessageT* const msg,
    ByteType const& msg_size
  );

  template <typename MessageT>
  PendingSendType sendMsg(
    NodeType const& dest, HandlerType const& han, MessageT* const msg
  );

  template <typename MessageT>
  PendingSendType sendMsg(
    NodeType const& dest, HandlerType const& han, MessageT* const msg,
    TagType const& tag
  );

  template <typename MsgT>
  PendingSendType sendMsg(
    NodeType const& dest, HandlerType const& han, MsgSharedPtr<MsgT> const& msg
  );

  template <typename MsgT>
  PendingSendType sendMsg(
    NodeType const& dest, HandlerType const& han, MsgSharedPtr<MsgT> const& msg,
    TagType const& tag
  );

  /*
   *  Auto method for dispatching to the serialization framework if required
   *  based on examining compile-time traits of the message
   */
  template <typename MessageT>
  PendingSendType sendMsgAuto(
    NodeType const& dest, HandlerType const& han, MessageT* const msg
  );

  template <typename MessageT>
  PendingSendType sendMsgAuto(
    NodeType const& dest, HandlerType const& han, MessageT* const msg,
    TagType const& tag
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
  PendingSendType broadcastMsgSz(
    MessageT* const msg, ByteType const& msg_size, TagType const& tag = no_tag
  );

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  PendingSendType broadcastMsg(
    MessageT* const msg, TagType const& tag = no_tag
  );

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  PendingSendType sendMsg(
    NodeType const& dest, MessageT* const msg, TagType const& tag = no_tag
  );

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  PendingSendType sendMsgSz(
    NodeType const& dest, MessageT* const msg, ByteType const& msg_size,
    TagType const& tag = no_tag
  );

  /*
   *  Auto method for dispatching to the serialization framework if required
   *  based on examining compile-time traits of the message
   */
  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  PendingSendType sendMsgAuto(
    NodeType const& dest, MessageT* const msg, TagType const& tag
  );

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  PendingSendType sendMsgAuto(
    NodeType const& dest, MessageT* const msg
  );

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  PendingSendType broadcastMsgAuto(
    MessageT* const msg, TagType const& tag = no_tag
  );
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
  PendingSendType broadcastMsg(
    MessageT* const msg, TagType const& tag = no_tag
  );

  template <ActiveFnType* f, typename MessageT>
  PendingSendType sendMsg(
    NodeType const& dest, MessageT* const msg, TagType const& tag = no_tag
  );

  template <ActiveFnType* f, typename MessageT>
  PendingSendType sendMsg(NodeType const& dest, MessageT* const msg);

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

  template <
    typename FunctorT,
    typename MessageT = typename util::FunctorExtractor<FunctorT>::MessageType
  >
  PendingSendType broadcastMsg(
    MessageT* const msg, TagType const& tag = no_tag
  );

  template <
    typename FunctorT,
    typename MessageT = typename util::FunctorExtractor<FunctorT>::MessageType
  >
  PendingSendType broadcastMsgAuto(
    MessageT* const msg, TagType const& tag = no_tag
  );

  template <
    typename FunctorT,
    typename MessageT = typename util::FunctorExtractor<FunctorT>::MessageType
  >
  PendingSendType broadcastMsgAuto(MessageT* const msg);

  template <
    typename FunctorT,
    typename MessageT = typename util::FunctorExtractor<FunctorT>::MessageType
  >
  PendingSendType sendMsg(
    NodeType const& dest, MessageT* const msg, TagType const& tag = no_tag
  );

  template <
    typename FunctorT,
    typename MessageT = typename util::FunctorExtractor<FunctorT>::MessageType
  >
  PendingSendType sendMsgAuto(
    NodeType const& dest, MessageT* const msg, TagType const& tag
  );

  template <
    typename FunctorT,
    typename MessageT = typename util::FunctorExtractor<FunctorT>::MessageType
  >
  PendingSendType sendMsgAuto(
    NodeType const& dest, MessageT* const msg
  );

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
  PendingSendType sendMsg(
    NodeType const& dest, HandlerType const& han, MessageT* const msg,
    UserSendFnType send_payload_fn
  );

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  PendingSendType sendMsg(
    NodeType const& dest, MessageT* const msg, UserSendFnType send_payload_fn
  );

  template <typename MessageT>
  PendingSendType broadcastMsg(
    HandlerType const& han, MessageT* const msg
  );

  template <typename MessageT>
  PendingSendType broadcastMsg(
    HandlerType const& han, MessageT* const msg, TagType const& tag
  );

  template <typename MsgT>
  PendingSendType broadcastMsg(
    HandlerType const& han, MsgSharedPtr<MsgT> const& msg
  );

  template <typename MsgT>
  PendingSendType broadcastMsg(
    HandlerType const& han, MsgSharedPtr<MsgT> const& msg, TagType const& tag
  );

  /*
   *  Auto method for dispatching to the serialization framework if required
   *  based on examining compile-time traits of the message
   */
  template <typename MessageT>
  PendingSendType broadcastMsgAuto(
    HandlerType const& han, MessageT* const msg
  );

  template <typename MessageT>
  PendingSendType broadcastMsgAuto(
    HandlerType const& han, MessageT* const msg, TagType const& tag
  );

  /*
   *----------------------------------------------------------------------------
   *                           End Send Data Message
   *----------------------------------------------------------------------------
   */

  void packMsg(
    MessageType const msg, MsgSizeType const& size, void* ptr,
    MsgSizeType const& ptr_bytes
  );

  SendDataRetType sendData(
    RDMA_GetType const& ptr, NodeType const& dest, TagType const& tag
  );

  bool recvDataMsgPriority(
    PriorityType priority, TagType const& tag, NodeType const& node,
    RDMA_ContinuationDeleteType next = nullptr
  );

  bool recvDataMsg(
    TagType const& tag, NodeType const& node,
    RDMA_ContinuationDeleteType next = nullptr
  );

  bool recvDataMsg(
    PriorityType priority, TagType const& tag, NodeType const& recv_node,
    bool const& enqueue, RDMA_ContinuationDeleteType next = nullptr
  );

  bool recvDataMsgBuffer(
    void* const user_buf, PriorityType priority, TagType const& tag,
    NodeType const& node = uninitialized_destination, bool const& enqueue = true,
    ActionType dealloc_user_buf = nullptr,
    RDMA_ContinuationDeleteType next = nullptr
  );

  bool recvDataMsgBuffer(
    void* const user_buf, TagType const& tag,
    NodeType const& node = uninitialized_destination, bool const& enqueue = true,
    ActionType dealloc_user_buf = nullptr,
    RDMA_ContinuationDeleteType next = nullptr
  );

  EventType sendMsgSized(
    MsgSharedPtr<BaseMsgType> const& msg, MsgSizeType const& msg_size
  );

  void performTriggeredActions();
  bool tryProcessIncomingActiveMsg();
  bool tryProcessDataMsgRecv();
  bool progress();

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

  HandlerType getCurrentHandler() const;
  NodeType getFromNodeCurrentHandler() const;
  EpochType getCurrentEpoch() const;

  #if backend_check_enabled(trace_enabled)
    trace::TraceEventIDType getCurrentTraceEvent() const;
  #endif

  PriorityType getCurrentPriority() const;
  PriorityLevelType getCurrentPriorityLevel() const;

  void scheduleActiveMsg(
    MsgSharedPtr<BaseMsgType> const& base, NodeType const& sender,
    MsgSizeType const& size, bool insert, ActionType cont = nullptr
  );

  bool processActiveMsg(
    MsgSharedPtr<BaseMsgType> const& base, NodeType const& sender,
    MsgSizeType const& size, bool insert, ActionType cont = nullptr
  );

  bool deliverActiveMsg(
    MsgSharedPtr<BaseMsgType> const& base, NodeType const& from_node,
    bool insert, ActionType cont
  );
  void deliverPendingMsgsHandler(
    HandlerType const& han, TagType const& tag = no_tag
  );
  void processMaybeReadyHanTag();

  EventType sendMsgBytes(
    NodeType const& dest, MsgSharedPtr<BaseMsgType> const& base,
    MsgSizeType const& msg_size, TagType const& send_tag
  );

  EventType sendMsgBytesWithPut(
    NodeType const& dest, MsgSharedPtr<BaseMsgType> const& base,
    MsgSizeType const& msg_size, TagType const& send_tag
  );

  /*
   * setGlobalEpoch() is a shortcut for both pushing and popping epochs on the
   * stack depending on the value of the `epoch' passed as an argument.
   */
  inline void setGlobalEpoch(EpochType const& epoch = no_epoch);
  /*
   * getGlobalEpoch() returns the top epoch on the stack iff epoch_stack.size()
   * > 0, else it returns no_epoch
   */
  inline EpochType getGlobalEpoch() const;

  /*
   * pushEpoch(epoch) pushes any epoch onto the local stack iff epoch !=
   * no_epoch; the epoch stack includes all locally pushed epochs and the
   * current contexts pushed, transitively causally related active message
   * handlers.
   */
  inline void pushEpoch(EpochType const& epoch);

  /*
   * popEpoch(epoch) shall remove the top entry from epoch_size_, iff the size
   * is non-zero and the `epoch' passed, if `epoch != no_epoch', is equal to the
   * top of the `epoch_stack_.top()'; else, it shall remove any entry from the
   * top of the stack.
   */
  inline EpochType popEpoch(EpochType const& epoch = no_epoch);

  /*
   * getEpoch() returns the top of the epoch_stack_
   */
  inline EpochType getEpoch() const;

  /*
   * Get/set the epoch for a message so an operation on it can be safely delayed
   */
  template <typename MsgT>
  inline EpochType getEpochContextMsg(MsgT* msg);

  template <typename MsgT>
  inline EpochType getEpochContextMsg(MsgSharedPtr<MsgT> const& msg);

  template <typename MsgT>
  inline EpochType setupEpochMsg(MsgT* msg);

  template <typename MsgT>
  inline EpochType setupEpochMsg(MsgSharedPtr<MsgT> const& msg);

  template <typename L>
  void addSendListener(std::unique_ptr<L> ptr) {
    send_listen_.push_back(std::move(ptr));
  }

  void clearListeners() {
    send_listen_.clear();
  }

private:
  bool testPendingActiveMsgAsyncRecv();
  bool testPendingDataMsgAsyncRecv();
  void finishPendingActiveMsgAsyncRecv(InProgressIRecv* irecv);
  void finishPendingDataMsgAsyncRecv(InProgressDataIRecv* irecv);

private:
  using EpochStackSizeType = typename EpochStackType::size_type;

  inline EpochStackSizeType epochPreludeHandler(EpochType const& epoch);
  inline void epochEpilogHandler(
    EpochType const& epoch, EpochStackSizeType const& prev_stack_size
  );

private:
# if backend_check_enabled(trace_enabled)
  trace::TraceEventIDType current_trace_context_ = trace::no_trace_event;
  trace::UserEventIDType trace_irecv             = trace::no_user_event_id;
  trace::UserEventIDType trace_isend             = trace::no_user_event_id;
  trace::UserEventIDType trace_irecv_polling_am  = trace::no_user_event_id;
  trace::UserEventIDType trace_irecv_polling_dm  = trace::no_user_event_id;
# endif

  HandlerType current_handler_context_                    = uninitialized_handler;
  NodeType current_node_context_                          = uninitialized_destination;
  EpochType current_epoch_context_                        = no_epoch;
  PriorityType current_priority_context_                  = no_priority;
  PriorityLevelType current_priority_level_context_       = no_priority_level;
  EpochType global_epoch_                                 = no_epoch;
  MaybeReadyType maybe_ready_tag_han_                     = {};
  ContWaitType pending_handler_msgs_                      = {};
  ContainerPendingType pending_recvs_                     = {};
  TagType cur_direct_buffer_tag_                          = starting_direct_buffer_tag;
  EpochStackType epoch_stack_;
  std::vector<ListenerType> send_listen_                  = {};
  IRecvHolder<InProgressIRecv> in_progress_active_msg_irecv;
  IRecvHolder<InProgressDataIRecv> in_progress_data_irecv;
  NodeType this_node_                                     = uninitialized_destination;
};

}} // end namespace vt::messaging

namespace vt {

using Active = messaging::ActiveMessenger;

extern messaging::ActiveMessenger* theMsg();

} // end namespace vt

#include "vt/messaging/active.impl.h"

#endif /*INCLUDED_MESSAGING_ACTIVE_H*/
