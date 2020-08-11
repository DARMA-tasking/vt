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
#include "vt/runtime/component/component_pack.h"

#if vt_check_enabled(trace_enabled)
  #include "vt/trace/trace_headers.h"
#endif

#include <type_traits>
#include <tuple>
#include <vector>
#include <unordered_map>
#include <limits>
#include <stack>

namespace vt { namespace messaging {

/** \file */

using MPI_TagType = int;

static constexpr TagType const PutPackedTag =
  std::numeric_limits<TagType>::max();

enum class MPITag : MPI_TagType {
  ActiveMsgTag = 1,
  DataMsgTag = 2
};

static constexpr TagType const starting_direct_buffer_tag = 1000;
static constexpr MsgSizeType const max_pack_direct_size = 512;

/**
 * \struct PendingRecv active.h vt/messaging/active.h
 *
 * \brief An pending receive event
 */
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

/**
 * \struct InProgressIRecv active.h vt/messaging/active.h
 *
 * \brief An in-progress MPI_Irecv watched by the runtime
 */
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

/**
 * \struct InProgressDataIRecv active.h vt/messaging/active.h
 *
 * \brief An in-progress pure data MPI_Irecv watched by the runtime
 */
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

/**
 * \struct BufferedActiveMsg active.h vt/messaging/active.h
 *
 * \brief Holds a buffered active message, used internally
 */
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

/**
 * \struct ActiveMessenger active.h vt/messaging/active.h
 *
 * \brief Core component of VT used to send messages.
 *
 * ActiveMessenger is a core VT component that provides the ability to send
 * messages \c Message to registered handlers. It manages the incoming and
 * outgoing messages using MPI to communicate \c MPI_Irecv and \c MPI_Isend
 *
 * There are various ways to send messages:
 *  - \ref typesafehan
 *  - \ref preregister
 *  - \ref basicsend
 *  - \ref functorsend
 *  - \ref sendpayload
 */
struct ActiveMessenger : runtime::component::PollableComponent<ActiveMessenger> {
  using BufferedMsgType      = BufferedActiveMsg;
  using MessageType          = ShortMessage;
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

  /**
   * \internal
   */
  ActiveMessenger();

  /**
   * \internal
   */
  virtual ~ActiveMessenger();


  std::string name() override { return "ActiveMessenger"; }

  /**
   * \brief Mark a message as a termination message.
   *
   * Used to ignore certain messages for the sake of termination detection
   * considering them control messages instead of normal message which are
   * tracked/counted by the termination detector.
   *
   * \param[in] msg the message to mark as a termination message
   */
  template <typename MsgPtrT>
  void markAsTermMessage(MsgPtrT const msg);

  /**
   * \brief Mark a message as a location message
   *
   * \param[in] msg the message to mark as a location message
   */
  template <typename MsgPtrT>
  void markAsLocationMessage(MsgPtrT const msg);

  /**
   * \brief Mark a message as a serialization control message
   *
   * \param[in] msg the message to mark as a serialization control message
   */
  template <typename MsgPtrT>
  void markAsSerialMsgMessage(MsgPtrT const msg);

  /**
   * \brief Mark a message as a collection message
   *
   * \param[in] msg the message to mark as a collection message
   */
  template <typename MsgPtrT>
  void markAsCollectionMessage(MsgPtrT const msg);

  /**
   * \brief Set the epoch in the envelope of a message
   *
   * \param[in] msg the message to mark the epoch on (envelope must be able to hold)
   * \param[in] epoch the epoch to mark on the message
   */
  template <typename MsgT>
  void setEpochMessage(MsgT* msg, EpochType epoch);

  /**
   * \internal
   * \brief Set the tag in the envelope of a message
   *
   * \param[in] msg the message to mark the tag on (envelope must be able to hold)
   * \param[in] tag the tag to mark on the message
   */
  template <typename MsgT>
  void setTagMessage(MsgT* msg, TagType tag);

  template <typename MsgPtrT>
  trace::TraceEventIDType makeTraceCreationSend(
    MsgPtrT msg, HandlerType const handler, auto_registry::RegistryTypeEnum type,
    MsgSizeType msg_size, bool is_bcast
  );

  // With serialization, the correct method is resolved via SFINAE.
  // This also includes additional guards to detect ambiguity.

  template <typename MessageT>
  ActiveMessenger::PendingSendType sendMsgSerializableImpl(
    NodeType dest,
    HandlerType han,
    MsgSharedPtr<MessageT>& msg,
    ByteType msg_size,
    TagType tag
  );

  // All messages that do NOT define their own serialization policy
  // and do NOT define their own serialization function are required
  // to be byte-transmittable. This covers basic byte-copyable
  // messages directly inheriting from ActiveMsg. ActivMsg implements
  // a serialize function which is implictly inherited..
  template <
    typename MessageT,
    std::enable_if_t<true
      and not ::vt::messaging::msg_defines_serialize_mode<MessageT>::value,
      int
    > = 0
  >
  inline ActiveMessenger::PendingSendType sendMsgImpl(
    NodeType dest,
    HandlerType han,
    MsgSharedPtr<MessageT>& msg,
    ByteType msg_size,
    TagType tag
  ) {
#ifndef vt_quirked_serialize_method_detection
    static_assert(
      not ::vt::messaging::has_own_serialize<MessageT>,
      "Message prohibiting serialization must not have a serialization function."
    );
#endif
    return sendMsgCopyableImpl<MessageT>(dest, han, msg, msg_size, tag);
  }

  // Serializable and serialization required on this type.
  template <
    typename MessageT,
    std::enable_if_t<true
      and ::vt::messaging::msg_defines_serialize_mode<MessageT>::value
      and ::vt::messaging::msg_serialization_mode<MessageT>::required,
      int
    > = 0
  >
  inline ActiveMessenger::PendingSendType sendMsgImpl(
    NodeType dest,
    HandlerType han,
    MsgSharedPtr<MessageT>& msg,
    ByteType msg_size,
    TagType tag
  ) {
#ifndef vt_quirked_serialize_method_detection
    static_assert(
      ::vt::messaging::has_own_serialize<MessageT>,
      "Message requiring serialization must have a serialization function."
    );
#endif
    return sendMsgSerializableImpl<MessageT>(dest, han, msg, msg_size, tag);
  }

  // Serializable, but support is only for derived types.
  // This type will still be sent using byte-copy serialization.
  template <
    typename MessageT,
    std::enable_if_t<true
      and ::vt::messaging::msg_defines_serialize_mode<MessageT>::value
      and ::vt::messaging::msg_serialization_mode<MessageT>::supported,
      int
    > = 0
  >
  inline ActiveMessenger::PendingSendType sendMsgImpl(
    NodeType dest,
    HandlerType han,
    MsgSharedPtr<MessageT>& msg,
    ByteType msg_size,
    TagType tag
  ) {
#ifndef vt_quirked_serialize_method_detection
    static_assert(
       ::vt::messaging::has_own_serialize<MessageT>,
       "Message supporting serialization must have a serialization function."
     );
#endif
    return sendMsgCopyableImpl<MessageT>(dest, han, msg, msg_size, tag);
  }

  // Messaged marked as prohibiting serialization cannot define
  // a serialization function and must be sent via byte-transmission.
  template <
    typename MessageT,
    std::enable_if_t<true
      and ::vt::messaging::msg_defines_serialize_mode<MessageT>::value
      and ::vt::messaging::msg_serialization_mode<MessageT>::prohibited,
      int
    > = 0
  >
  inline ActiveMessenger::PendingSendType sendMsgImpl(
    NodeType dest,
    HandlerType han,
    MsgSharedPtr<MessageT>& msg,
    ByteType msg_size,
    TagType tag
  ) {
#ifndef vt_quirked_serialize_method_detection
    static_assert(
      not ::vt::messaging::has_own_serialize<MessageT>,
      "Message prohibiting serialization must not have a serialization function."
    );
#endif
    return sendMsgCopyableImpl<MessageT>(dest, han, msg, msg_size, tag);
  }

  template <typename MessageT>
  ActiveMessenger::PendingSendType sendMsgCopyableImpl(
    NodeType dest,
    HandlerType han,
    MsgSharedPtr<MessageT>& msg,
    ByteType msg_size,
    TagType tag
  );

  /**
   * \defgroup preregister Basic Active Message Send with Pre-Registered Handler
   *
   * \brief Send a message to pre-registered active message handler.
   *
   * \code{.cpp}
   *   struct MyMsg : vt::Message {
   *     explicit MyMsg(int in_a) : a_(in_a) { }
   *     int a_;
   *   };
   *
   *   void myHandler(MyMsg* msg) {
   *     // do work ...
   *   }
   *
   *   void sendCode() {
   *     // Explicit user registration of the handler, must be ordered as a
   *     // collective invocation across nodes
   *     HandlerType const han = registerNewHandler(my_handler);
   *
   *     auto msg = makeMessage<MyMsg>(156);
   *     theMsg()->sendMsg(29, han, msg.get());
   *   }
   * \endcode
   * @{
   */

  /**
   * \brief Send a message with a pre-registered handler.
   *
   * Only invoke this variant if you know the size or the \c sizeof(Message) is
   * different than the number of bytes you actually want to send
   *
   * \param[in] dest node to send the message to
   * \param[in] han handler to send the message to
   * \param[in] msg the message to send
   * \param[in] msg_size the size of the message being sent
   * \param[in] tag the tag to put on the message
   *
   * \return the \c PendingSend for the send
   */
  template <typename MessageT>
  PendingSendType sendMsgSz(
    NodeType dest,
    HandlerType han,
    MessageT* msg,
    ByteType msg_size,
    TagType tag = no_tag
  );

  /**
   * \brief Send a message with a pre-registered handler.
   *
   * \param[in] dest node to send the message to
   * \param[in] han handler to send the message to
   * \param[in] msg the message to send
   * \param[in] tag the tag to put on the message
   *
   * \return the \c PendingSend for the send
   */
  template <typename MessageT>
  PendingSendType sendMsg(
    NodeType dest,
    HandlerType han,
    MessageT* msg,
    TagType tag = no_tag
  );

  /**
   * \brief Send a message with a pre-registered handler.
   *
   * \param[in] dest node to send the message to
   * \param[in] han handler to send the message to
   * \param[in] msg the message to send (shared ptr)
   * \param[in] tag the tag to put on the message
   *
   * \return the \c PendingSend for the send
   */
  template <typename MsgT>
  PendingSendType sendMsg(
    NodeType dest,
    HandlerType han,
    MsgSharedPtr<MsgT>& msg,
    TagType tag = no_tag
  );

  /**
   * \brief Send a message with a pre-registered handler.
   *
   * \deprecated Use \p sendMessage instead.
   *
   * \param[in] dest the destination node to send the message to
   * \param[in] han the handler to invoke
   * \param[in] msg the message to send
   * \param[in] tag the tag to put on the message
   *
   * \return the \c PendingSend for the send
   */
  template <typename MessageT>
  PendingSendType sendMsgAuto(
    NodeType dest,
    HandlerType han,
    MessageT* msg,
    TagType tag = no_tag
  );

  /** @} */

  /*
   *----------------------------------------------------------------------------
   *          End Basic Active Message Send with Pre-Registered Handler
   *----------------------------------------------------------------------------
   */

  /**
   * \defgroup typesafehan Send Message Active Function (type-safe handler)
   *
   * \brief Send a message to a auto-registered type-safe active message
   * handler. This is the predominant way that the messenger is expected to be
   * used.
   *
   * \code{.cpp}
   *   struct MyMsg : vt::Message {
   *     explicit MyMsg(int in_a) : a_(in_a) { }
   *     int a_;
   *   };
   *
   *   void myHandler(MyMsg* msg) {
   *     // do work ...
   *   }
   *
   *   void sendCode() {
   *     // myHandler is automatically registered with the overload
   *     theMsg()->sendMsg<MyMsg, myHandler>(1, msg);
   *   }
   * \endcode
   *
   * @{
   */

  /**
   * \brief Broadcast a message with an explicit size.
   *
   * Use this variant to broadcast a message when \c sizeof(Message) != the
   * actual size you want to send (e.g., extra bytes on the end)
   *
   * \param[in] msg the message to broadcast
   * \param[in] msg_size the size of the message to send
   * \param[in] tag the tag to put on the message
   *
   * \return the \c PendingSend for the sent message
   */
  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  PendingSendType broadcastMsgSz(
    MessageT* msg,
    ByteType msg_size,
    TagType tag = no_tag
  );

  /**
   * \brief Broadcast a message.
   *
   * \param[in] msg the message to broadcast
   * \param[in] tag the tag to put on the message
   *
   * \return the \c PendingSend for the sent message
   */
  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  PendingSendType broadcastMsg(
    MessageT* msg,
    TagType tag = no_tag
  );

  /**
   * \brief Send a message.
   *
   * \param[in] dest the destination node to send the message to
   * \param[in] msg the message to send
   * \param[in] tag the tag to put on the message
   *
   * \return the \c PendingSend for the sent message
   */
  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  PendingSendType sendMsg(
    NodeType dest,
    MessageT* msg,
    TagType tag = no_tag
  );

  /**
   * \brief Send a message with explicit size.
   *
   * Invoke this send variant if you know the size or the \c sizeof(Message) is
   * different than the number of bytes you actually want to send (e.g., extra
   * bytes on the end of the message)
   *
   * \param[in] dest node to send the message to
   * \param[in] msg the message to send
   * \param[in] msg_size the size of the message being sent
   * \param[in] tag the tag to put on the message
   *
   * \return the \c PendingSend for the send
   */
  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  PendingSendType sendMsgSz(
    NodeType dest,
    MessageT* msg,
    ByteType msg_size,
    TagType tag = no_tag
  );

  /**
   * \brief Broadcast a message.
   *
   * \deprecated Use \b broadcastMsg instead.
   *
   * \param[in] msg the message to broadcast
   * \param[in] tag the tag to put on the message
   *
   * \return the \c PendingSend for the send
   */
  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  PendingSendType broadcastMsgAuto(
    MessageT* msg,
    TagType tag = no_tag
  );

  /**
   * \brief Send a message.
   *
   * \deprecated Use \b sendMsg instead.
   *
   * \param[in] dest the destination node to send the message to
   * \param[in] msg the message to send
   * \param[in] tag the tag to put on the message
   *
   * \return the \c PendingSend for the send
   */
  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  PendingSendType sendMsgAuto(
    NodeType dest,
    MessageT* msg,
    TagType tag = no_tag
  );

  /** @} */

  /*
   *----------------------------------------------------------------------------
   *             End Send Message Active Function (type-safe handler)
   *----------------------------------------------------------------------------
   */

  /**
   * \defgroup basicsend Send Message BASIC Active Function
   *
   * \brief Send a message to a auto-registered non-type-safe active message
   * handler. This should be used rarely.
   *
   * Send message using basic function handler. These handlers are NOT type-safe
   * and require the user to cast their message to the correct type as
   * shown. Most likely this will be deprecated unless there is a use for this,
   * since type safety does not cost anything in terms of overhead (either at
   * runtime or compile-time).
   *
   * \code{.cpp}
   *   struct MyMsg : vt::Message {
   *     explicit MyMsg(int in_a) : a_(in_a) { }
   *     int a_;
   *   };
   *
   *   void basicHandler(vt::BaseMessage* msg_in) {
   *     MyMsg* msg = static_cast<MyMsg*>(msg_in);
   *     ...
   *   }
   *
   *   void sendCode() {
   *     // myHandler is automatically registered with the overload
   *     theMsg()->sendMsg<basicHandler, MyMsg>(1, msg);
   *   }
   * \endcode
   *
   * @{
   */

  /**
   * \brief Broadcast a message with a type-safe handler.
   *
   * \param[in] msg the message to broadcast
   * \param[in] tag the optional tag to put on the message
   *
   * \return the \c PendingSend for the broadcast
   */
  template <ActiveFnType* f, typename MessageT>
  PendingSendType broadcastMsg(
    MessageT* msg,
    TagType tag = no_tag
  );

  /**
   * \brief Send a message with a type-safe handler.
   *
   * \param[in] dest the destination node to send the message to
   * \param[in] msg the message to broadcast
   * \param[in] tag the optional tag to put on the message
   *
   * \return the \c PendingSend for the broadcast
   */
  template <ActiveFnType* f, typename MessageT>
  PendingSendType sendMsg(
    NodeType dest,
    MessageT* msg,
    TagType tag = no_tag
  );

  /** @} */

  /*
   *----------------------------------------------------------------------------
   *              End Send Message BASIC Active Function (deprecated?)
   *----------------------------------------------------------------------------
   */

  /**
   * \defgroup functorsend Send Message to Functor Variants
   *
   * \brief Send a message to an auto-registered functor-type handler
   *
   * Send message using functor handler. These handlers are type-safe and do not
   * require the user to specify the message because it can be automatically
   * detected.
   *
   * \code{.cpp}
   *   struct MyMsg : vt::Message {
   *     explicit MyMsg(int in_a) : a_(in_a) { }
   *     int a_;
   *   };
   *
   *   struct X {
   *     void operator()(MyMsg* msg) const { ... };
   *   };
   *
   *   void sendCode() {
   *     // X is automatically registered with the overload and the message
   *     // type is automatically detected
   *     theMsg()->sendMsg<X>(1, msg);
   *   }
   * \endcode
   *
   * @{
   */

  /**
   * \brief Broadcast a message.
   *
   * \param[in] msg the message to broadcast
   * \param[in] tag the optional tag to put on the message
   *
   * \return the \c PendingSend for the broadcast
   */
  template <
    typename FunctorT,
    typename MessageT = typename util::FunctorExtractor<FunctorT>::MessageType
  >
  PendingSendType broadcastMsg(
    MessageT* msg,
    TagType tag = no_tag
  );

  /**
   * \brief Broadcast a message.
   *
   * \deprecated Use \p broadcastMsg instead.
   *
   * \param[in] msg the message to send
   * \param[in] tag the optional tag to put on the message
   *
   * \return the \c PendingSend for the broadcast
   */
  template <
    typename FunctorT,
    typename MessageT = typename util::FunctorExtractor<FunctorT>::MessageType
  >
  PendingSendType broadcastMsgAuto(
    MessageT* msg,
    TagType tag = no_tag
  );

  /**
   * \brief Send a message with a type-safe handler.
   *
   * \param[in] dest the destination node to send the message to
   * \param[in] msg the message to broadcast
   * \param[in] tag the tag to put on the message
   *
   * \return the \c PendingSend for the send
   */
  template <
    typename FunctorT,
    typename MessageT = typename util::FunctorExtractor<FunctorT>::MessageType
  >
  PendingSendType sendMsg(
    NodeType dest,
    MessageT* msg,
    TagType tag = no_tag
  );

  /**
   * \brief Send a message.
   *
   * \deprecated Use \p sendMsg instead.
   *
   * \param[in] dest the destination node to send the message to
   * \param[in] msg the message to broadcast
   * \param[in] tag the optional tag to put on the message
   *
   * \return the \c PendingSend for the send
   */
  template <
    typename FunctorT,
    typename MessageT = typename util::FunctorExtractor<FunctorT>::MessageType
  >
  PendingSendType sendMsgAuto(
    NodeType dest,
    MessageT* msg,
    TagType tag = no_tag
  );

  /** @} */

  /*
   *----------------------------------------------------------------------------
   *                      End Send Message Functor Variants
   *----------------------------------------------------------------------------
   */

  /**
   * \defgroup sendpayload Send Data Message with data payload
   *
   * \brief Send message that includes a payload of data
   *
   * Send message that includes a payload that can be arbitrary data that is
   * coordinated by the system
   *
   * \code{.cpp}
   *   struct PutMessage : vt::Message {
   *     PutMessage() { }
   *     vt::TagType mpi_tag_to_recv = vt::no_tag;
   *   };
   *
   *   void myHandler(PutMessage* msg) {
   *     NodeType send_node = 0;
   *     theMsg()->recvDataMsg(
   *       msg->mpi_tag_to_recv, send_node,
   *       [=](RDMA_GetType ptr, ActionType deleter){
   *          // do something with ptr
   *          deleter();
   *       }
   *     );
   *   }
   *
   *   void sendCode() {
   *     NodeType put_node = 1;
   *     // The user's payload function that invokes the system send function
   *     // passed to the lambda
   *     auto send_payload = [&](Active::SendFnType send){
   *       auto ret = send(vt::RDMA_GetType{ptr, num_bytes}, put_node, vt::no_tag);
   *       msg->mpi_tag_to_recv = std::get<1>(ret);
   *     };
   *     theMsg()->sendMsg<PutMessage, myHandler>(1, msg);
   *   }
   * \endcode
   *
   * @{
   */

  /**
   * \brief Broadcast a message.
   *
   * \param[in] han the handler to invoke
   * \param[in] msg the message to broadcast
   * \param[in] tag the optional tag to put on the message
   *
   * \return the \c PendingSend for the send
   */
  template <typename MessageT>
  PendingSendType broadcastMsg(
    HandlerType han,
    MessageT* msg,
    TagType tag = no_tag
  );

  /**
   * \brief Broadcast a message.
   *
   * \param[in] han the handler to invoke
   * \param[in] msg the message to broadcast
   * \param[in] tag the optional tag to put on the message
   *
   * \return the \c PendingSend for the send
   */
  template <typename MsgT>
  PendingSendType broadcastMsg(
    HandlerType han,
    MsgSharedPtr<MsgT>& msg,
    TagType tag = no_tag
  );

  /**
   * \brief Send a message with a special payload function.
   *
   * \param[in] dest the destination node to send the message to
   * \param[in] han the handler to invoke
   * \param[in] msg the message to send
   * \param[in] send_payload_fn
   *
   * \return the \c PendingSend for the send
   */
  template <typename MessageT>
  PendingSendType sendMsg(
    NodeType dest,
    HandlerType han,
    MessageT* msg,
    UserSendFnType send_payload_fn
  );

  /**
   * \brief Send a message with a special payload function.
   *
   * \param[in] dest the destination node to send the message to
   * \param[in] msg the message to send
   * \param[in] send_payload_fn
   *
   * \return the \c PendingSend for the send
   */
  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  PendingSendType sendMsg(
    NodeType dest,
    MessageT* msg,
    UserSendFnType send_payload_fn
  );

  /**
   * \brief Broadcast a message.
   *
   * \deprecated Use \p broadcastMsg instead.
   *
   * \param[in] han the handler to invoke
   * \param[in] msg the message to broadcast
   * \param[in] tag the optional tag to put on the message
   *
   * \return the \c PendingSend for the send
   */
  template <typename MessageT>
  PendingSendType broadcastMsgAuto(
    HandlerType han,
    MessageT* msg,
    TagType tag = no_tag
  );

  /** @} */

  /*
   *----------------------------------------------------------------------------
   *                           End Send Data Message
   *----------------------------------------------------------------------------
   */

  /**
   * \internal
   * \brief Pack a message, used by the system
   *
   * Packs bytes directly after a message into a single contiguous buffer to
   * send. The message better have sufficient space allocated after the message
   * to pack these bytes. This is often used when the memory pool slot selected
   * comes with some extra space instead of sending two separate buffers.
   *
   * \param[in] msg message to pack
   * \param[in] size size of message
   * \param[in] ptr pointer to pack
   * \param[in] ptr_bytes bytes at pointer to pack
   */
  void packMsg(
    MessageType* msg, MsgSizeType size, void* ptr, MsgSizeType ptr_bytes
  );

  /**
   * \internal
   * \brief Send raw bytes to a node
   *
   * \param[in] ptr the pointer and bytes to send
   * \param[in] dest the destination node
   * \param[in] tag the MPI tag put on the send (if vt::no_tag, increments tag)
   *
   * \return a tuple with the event ID and tag used
   */
  SendDataRetType sendData(
    RDMA_GetType const& ptr, NodeType const& dest, TagType const& tag
  );

  /**
   * \internal
   * \brief Receive data as bytes from a node with a priority
   *
   * \param[in] priority the priority to receive the data
   * \param[in] tag the MPI tag to receive on
   * \param[in] node the node from which to receive
   * \param[in] next a continuation to execute when data arrives
   *
   * \return whether it was successful or pending
   */
  bool recvDataMsgPriority(
    PriorityType priority, TagType const& tag, NodeType const& node,
    RDMA_ContinuationDeleteType next = nullptr
  );

  /**
   * \internal
   * \brief Receive data as bytes from a node
   *
   * \param[in] tag the MPI tag to receive on
   * \param[in] node the node from which to receive
   * \param[in] next a continuation to execute when data arrives
   *
   * \return whether it was successful or pending
   */
  bool recvDataMsg(
    TagType const& tag, NodeType const& node,
    RDMA_ContinuationDeleteType next = nullptr
  );

  /**
   * \internal
   * \brief Receive data as bytes from a node with a priority
   *
   * \param[in] priority the priority to receive the data
   * \param[in] tag the MPI tag to receive on
   * \param[in] recv_node the node from which to receive
   * \param[in] enqueue whether to enqueue the pending receive
   * \param[in] next a continuation to execute when data arrives
   *
   * \return whether it was successful or pending
   */
  bool recvDataMsg(
    PriorityType priority, TagType const& tag, NodeType const& recv_node,
    bool const& enqueue, RDMA_ContinuationDeleteType next = nullptr
  );

  /**
   * \internal
   * \brief Receive data as bytes with a buffer and priority
   *
   * \param[in] user_buf the buffer to receive into
   * \param[in] priority the priority for the operation
   * \param[in] tag the MPI tag to use for receive
   * \param[in] node the node receiving from
   * \param[in] enqueue whether to enqueue the operation
   * \param[in] dealloc_user_buf the action to deallocate a user buffer
   * \param[in] next the continuation when the data is ready
   *
   * \return whether the data is ready or pending
   */
  bool recvDataMsgBuffer(
    void* const user_buf, PriorityType priority, TagType const& tag,
    NodeType const& node = uninitialized_destination, bool const& enqueue = true,
    ActionType dealloc_user_buf = nullptr,
    RDMA_ContinuationDeleteType next = nullptr
  );

  /**
   * \internal
   * \brief Receive data as bytes with a buffer
   *
   * \param[in] user_buf the buffer to receive into
   * \param[in] tag the MPI tag to use for receive
   * \param[in] node the node receiving from
   * \param[in] enqueue whether to enqueue the operation
   * \param[in] dealloc_user_buf the action to deallocate a user buffer
   * \param[in] next the continuation when the data is ready
   *
   * \return whether the data is ready or pending
   */
  bool recvDataMsgBuffer(
    void* const user_buf, TagType const& tag,
    NodeType const& node = uninitialized_destination, bool const& enqueue = true,
    ActionType dealloc_user_buf = nullptr,
    RDMA_ContinuationDeleteType next = nullptr
  );

  /**
   * \internal
   * \brief Low-level send of a message, handler and other control data should
   * be set already
   *
   * \param[in] msg the message to send
   * \param[in] msg_size the size of the message (in bytes)
   *
   * \return the event for tracking the send completion
   */
  EventType doMessageSend(
    MsgSharedPtr<BaseMsgType>& msg, MsgSizeType msg_size
  );

  /**
   * \internal
   * \brief Poll MPI to discover an incoming message with a handler
   *
   * \return whether a message was found
   */
  bool tryProcessIncomingActiveMsg();

  /**
   * \internal
   * \brief Poll MPI for raw data messages
   *
   * \return whether a message was found
   */
  bool tryProcessDataMsgRecv();

  /**
   * \internal
   * \brief Call into the progress engine
   *
   * \return whether any action was taken (progress was made)
   */
  int progress() override;

  /**
   * \internal
   * \brief Register a bare handler
   *
   * \param[in] fn the function to register
   * \param[in] tag the tag this handler will accept (\c vt::no_tag means any)
   *
   * \return the handler ID
   */
  HandlerType registerNewHandler(
    ActiveClosureFnType fn, TagType const& tag = no_tag
  );

  /**
   * \internal
   * \brief Swap the underlying handler function pointer
   *
   * \param[in] han the handler to swap function pointers
   * \param[in] fn the new function pointer
   * \param[in] tag the tag this handler will accept (\c vt::no_tag means any)
   */
  void swapHandlerFn(
    HandlerType const& han, ActiveClosureFnType fn, TagType const& tag = no_tag
  );

  /**
   * \internal
   * \brief Un-register a bare handler
   *
   * \param[in] han the handler to de-register
   * \param[in] tag the tag this handler will accept (\c vt::no_tag means any)
   */
  void unregisterHandlerFn(HandlerType const& han, TagType const& tag = no_tag);

  /**
   * \internal
   * \brief Register a handler function for existing handler
   *
   * \param[in] han the handler to swap function pointers
   * \param[in] fn the new function pointer
   * \param[in] tag the tag this handler will accept (\c vt::no_tag means any)
   */
  void registerHandlerFn(
    HandlerType const& han, ActiveClosureFnType fn, TagType const& tag = no_tag
  );

  /**
   * \internal
   * \brief Register an active handler (collective)
   *
   * \param[in] fn the function pointer for the handler
   * \param[in] tag the tag this handler will accept (\c vt::no_tag means any)
   *
   * \return the handler ID
   */
  HandlerType collectiveRegisterHandler(
    ActiveClosureFnType fn, TagType const& tag = no_tag
  );

  /**
   * \internal
   * \brief Get the current handler (valid only while running a handler)
   *
   * \return the handler ID
   */
  HandlerType getCurrentHandler() const;

  /**
   * \internal
   * \brief Get the from node for the current running handler (valid only while
   * running a handler)
   *
   * For the current handler that is executing, get the node that sent the
   * message that caused this handler to run. Note, for collection handlers this
   * will not be the logical node that sent the message. It will be the node
   * that last forwarded the message during location discovery.
   *
   * \return the node that sent the message that triggered the current handler
   */
  NodeType getFromNodeCurrentHandler() const;

  /**
   * \internal
   * \brief Get the current epoch on the handler running
   *
   * \return the epoch on the message that triggered the current handler
   */
  EpochType getCurrentEpoch() const;

  #if vt_check_enabled(trace_enabled)
    /**
     * \internal
     * \brief Get the trace event on the handler running
     *
     * \return the trace event on the message that triggered the current handler
     */
    trace::TraceEventIDType getCurrentTraceEvent() const;
  #endif

  /**
   * \internal
   * \brief Get the priority on the handler running
   *
   * \return the priority on the message that triggered the current handler
   */
  PriorityType getCurrentPriority() const;

  /**
   * \internal
   * \brief Get the priority level on the handler running
   *
   * \return the priority level of the message that triggered the current handler
   */
  PriorityLevelType getCurrentPriorityLevel() const;

  /**
   * \internal
   * \brief Schedule an active message for future delivery
   *
   * \param[in] base the message ptr
   * \param[in] sender the sender of the message
   * \param[in] size the size of the message
   * \param[in] insert whether to insert the message if handler does not exist
   * \param[in] cont continuation after message is processed
   */
  void scheduleActiveMsg(
    MsgSharedPtr<BaseMsgType> const& base, NodeType const& sender,
    MsgSizeType const& size, bool insert, ActionType cont = nullptr
  );

  /**
   * \internal
   * \brief Process an incoming active message
   *
   * Forwards the message to the appropriate group nodes or broadcasts it
   * depending on envelope. May deliver the message locally or just forward it
   * depending on the target.
   *
   * \param[in] base the message ptr
   * \param[in] sender the sender of the message
   * \param[in] size the size of the message
   * \param[in] insert whether to insert the message if handler does not exist
   * \param[in] cont continuation after message is processed
   *
   * \return whether it was delivered locally
   */
  bool processActiveMsg(
    MsgSharedPtr<BaseMsgType> const& base, NodeType const& sender,
    MsgSizeType const& size, bool insert, ActionType cont = nullptr
  );

  /**
   * \internal
   * \brief Deliver an active message locally
   *
   * \param[in] base the message ptr
   * \param[in] from_node the node the message came from
   * \param[in] insert whether to insert the message if handler does not exist
   * \param[in] cont continuation after message is processed
   *
   * \return whether the message was delivered, false when handler does not exist
   */
  bool deliverActiveMsg(
    MsgSharedPtr<BaseMsgType> const& base, NodeType const& from_node,
    bool insert, ActionType cont
  );

  /**
   * \internal
   * \brief Deliver pending messaging waiting for a handler to be registered
   *
   * \param[in] han the handler that will now accept
   * \param[in] tag the tag for that handler
   */
  void deliverPendingMsgsHandler(
    HandlerType const& han, TagType const& tag = no_tag
  );

  /**
   * \internal
   * \brief Process any messages that might be ready now (handler is now
   * registered)
   */
  void processMaybeReadyHanTag();

  /**
   * \internal
   * \brief Send message as low-level bytes after packing put bytes if needed
   *
   * \param[in] dest the destination of the message
   * \param[in] base the message base pointer
   * \param[in] msg_size the size of the message
   * \param[in] send_tag the send tag on the message
   *
   * \return the event to test/wait for completion
   */
  EventType sendMsgBytes(
    NodeType const& dest, MsgSharedPtr<BaseMsgType> const& base,
    MsgSizeType const& msg_size, TagType const& send_tag
  );

  /**
   * \internal
   * \brief Send message as low-level bytes that is already packed
   *
   * \param[in] dest the destination of the message
   * \param[in] base the message base pointer
   * \param[in] msg_size the size of the message
   * \param[in] send_tag the send tag on the message
   *
   * \return the event to test/wait for completion
   */
  EventType sendMsgBytesWithPut(
    NodeType const& dest, MsgSharedPtr<BaseMsgType> const& base,
    MsgSizeType const& msg_size, TagType const& send_tag
  );

  /**
   * \internal
   * \brief Set the global epoch (\c pushEpoch is more desirable)
   *
   * \c setGlobalEpoch() is a shortcut for both pushing and popping epochs on the
   * stack depending on the value of the `epoch' passed as an argument.
   *
   * \param[in] epoch the epoch to set
   */
  inline void setGlobalEpoch(EpochType const& epoch = no_epoch);

  /**
   * \internal
   * \brief Get the current global epoch
   *
   * \c Returns the top epoch on the stack iff \c epoch_stack.size() > 0, else it
   * returns \c vt::no_epoch
   *
   * \return the current global epoch
   */
  inline EpochType getGlobalEpoch() const;

  /**
   * \internal
   * \brief Push an epoch on the stack
   *
   * Pushes any epoch onto the local stack iff epoch != no_epoch; the epoch
   * stack includes all locally pushed epochs and the current contexts pushed,
   * transitively causally related active message handlers.
   *
   * \param[in] epoch the epoch to push
   */
  inline void pushEpoch(EpochType const& epoch);

  /**
   * \internal
   * \brief Pop an epoch off the stack
   *
   * Shall remove the top entry from epoch_size_, iff the size
   * is non-zero and the `epoch' passed, if `epoch != no_epoch', is equal to the
   * top of the `epoch_stack_.top()'; else, it shall remove any entry from the
   * top of the stack.
   *
   * \param[in] epoch the epoch that is expected to exist on the stack
   *
   * \return the epoch popped off the stack
   *
   */
  inline EpochType popEpoch(EpochType const& epoch = no_epoch);

  /**
   * \internal
   * \brief Returns the top of the epoch stack
   *
   * \return the epoch on the top of the stack
   */
  inline EpochType getEpoch() const;

  /**
   * \internal
   * \brief Get the epoch for a message based on the current context so an
   * subsequent operation on it can be safely delayed
   *
   * \param[in] msg the message to set the epoch on
   *
   * \return the epoch on the message or from context
   */
  template <typename MsgT>
  inline EpochType getEpochContextMsg(MsgT* msg);

  /**
   * \internal
   * \brief Get the epoch for a message based on the current context so an
   * subsequent operation on it can be safely delayed
   *
   * \param[in] msg the message (shared ptr) to set the epoch on
   *
   * \return the epoch on the message or from context
   */
  template <typename MsgT>
  inline EpochType getEpochContextMsg(MsgSharedPtr<MsgT> const& msg);

  /**
   * \internal
   * \brief Set the epoch on a message.
   *
   * The method finds the current epoch based on whether its already set on the
   * message (which in that case it uses the one on the message already) or
   * obtains the current epoch from the epoch stack based on the handler that is
   * running
   *
   * \param[in,out] msg the message to get/set the epoch on
   *
   * \return the epoch set
   */
  template <typename MsgT>
  inline EpochType setupEpochMsg(MsgT* msg);

  /**
   * \internal
   * \brief Set the epoch on th message
   *
   * \param[in] msg the message to get/set the epoch on
   *
   * \return the epoch set
   */
  template <typename MsgT>
  inline EpochType setupEpochMsg(MsgSharedPtr<MsgT> const& msg);

  /**
   * \internal
   * \brief Register a listener on the active messenger---see \c Listener
   *
   * \param[in] ptr a \c std::unique_ptr<L> to a listener
   */
  template <typename L>
  void addSendListener(std::unique_ptr<L> ptr) {
    send_listen_.push_back(std::move(ptr));
  }

  /**
   * \internal
   * \brief Clear all listeners
   */
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
# if vt_check_enabled(trace_enabled)
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
