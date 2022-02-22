/*
//@HEADER
// *****************************************************************************
//
//                                   active.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_MESSAGING_ACTIVE_H
#define INCLUDED_VT_MESSAGING_ACTIVE_H

#include <cstdint>
#include <memory>
#include <mpi.h>

#include "vt/config.h"
#include "vt/activefn/activefn.h"
#include "vt/messaging/active.fwd.h"
#include "vt/messaging/message/smart_ptr.h"
#include "vt/messaging/pending_send.h"
#include "vt/messaging/request_holder.h"
#include "vt/messaging/send_info.h"
#include "vt/messaging/async_op_wrapper.h"
#include "vt/event/event.h"
#include "vt/registry/registry.h"
#include "vt/registry/auto/auto_registry_interface.h"
#include "vt/trace/trace_common.h"
#include "vt/utils/static_checks/functor.h"
#include "vt/runtime/component/component_pack.h"
#include "vt/elm/elm_id.h"
#include "vt/elm/elm_stats.h"

#if vt_check_enabled(trace_enabled)
  #include "vt/trace/trace_headers.h"
#endif

#include <type_traits>
#include <tuple>
#include <vector>
#include <unordered_map>
#include <limits>
#include <stack>

namespace vt {

/// A pair of a void* and number of bytes (length) for sending data
using PtrLenPairType = std::tuple<void*, ByteType>;

/// A continuation function with an allocated pointer with a deleter function
using ContinuationDeleterType =
  std::function<void(PtrLenPairType data, ActionType deleter)>;

} /* end namespace vt */

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
  int nchunks = 0;
  void* user_buf = nullptr;
  ContinuationDeleterType cont = nullptr;
  ActionType dealloc_user_buf = nullptr;
  NodeType sender = uninitialized_destination;
  PriorityType priority = no_priority;
  bool is_user_buf = false;

  PendingRecv(
    int in_nchunks, void* in_user_buf, ContinuationDeleterType in_cont,
    ActionType in_dealloc_user_buf, NodeType node,
    PriorityType in_priority, bool in_is_user_buf
  ) : nchunks(in_nchunks), user_buf(in_user_buf), cont(in_cont),
      dealloc_user_buf(in_dealloc_user_buf), sender(node),
      priority(in_priority), is_user_buf(in_is_user_buf)
  { }

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | nchunks
      | user_buf
      | cont
      | dealloc_user_buf
      | sender
      | priority
      | is_user_buf;
  }
};

/**
 * \struct InProgressBase active.h vt/messaging/active.h
 *
 * \brief Base class for an in-progress MPI operation
 */
struct InProgressBase {
  InProgressBase(
    char* in_buf, MsgSizeType in_probe_bytes, NodeType in_sender
  ) : buf(in_buf), probe_bytes(in_probe_bytes), sender(in_sender),
      valid(true)
  { }

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | buf
      | probe_bytes
      | sender
      | valid;
  }

  char* buf = nullptr;
  MsgSizeType probe_bytes = 0;
  NodeType sender = uninitialized_destination;
  bool valid = false;
};

/**
 * \struct InProgressIRecv active.h vt/messaging/active.h
 *
 * \brief An in-progress MPI_Irecv watched by the runtime
 */
struct InProgressIRecv : InProgressBase {

  InProgressIRecv(
    char* in_buf, MsgSizeType in_probe_bytes, NodeType in_sender,
    MPI_Request in_req = MPI_REQUEST_NULL
  ) : InProgressBase(in_buf, in_probe_bytes, in_sender),
      req(in_req)
  { }

  bool test(int& num_mpi_tests) {
    VT_ALLOW_MPI_CALLS; // MPI_Test

    int flag = 0;
    MPI_Status stat;
    MPI_Test(&req, &flag, &stat);
    num_mpi_tests++;
    return flag;
  }

private:
  MPI_Request req = MPI_REQUEST_NULL;
};

/**
 * \struct InProgressDataIRecv active.h vt/messaging/active.h
 *
 * \brief An in-progress pure data MPI_Irecv watched by the runtime
 */
struct InProgressDataIRecv : InProgressBase {
  InProgressDataIRecv(
    char* in_buf, MsgSizeType in_probe_bytes, NodeType in_sender,
    std::vector<MPI_Request> in_reqs, void* const in_user_buf,
    ActionType in_dealloc_user_buf,
    ContinuationDeleterType in_next,
    PriorityType in_priority
  ) : InProgressBase{in_buf, in_probe_bytes, in_sender},
      user_buf(in_user_buf), dealloc_user_buf(in_dealloc_user_buf),
      next(in_next), priority(in_priority), reqs(std::move(in_reqs))
  { }

  bool test(int& num_mpi_tests) {
    int flag = 0;
    MPI_Status stat;
    for ( ; cur < reqs.size(); cur++) {
      VT_ALLOW_MPI_CALLS; // MPI_Test

      MPI_Test(&reqs[cur], &flag, &stat);
      num_mpi_tests++;

      if (flag == 0) {
        return false;
      }
    }

    return true;
  }

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | user_buf
      | dealloc_user_buf
      | next
      | priority
      | cur
      | reqs;
  }

  void* user_buf = nullptr;
  ActionType dealloc_user_buf = nullptr;
  ContinuationDeleterType next = nullptr;
  PriorityType priority = no_priority;

private:
  std::size_t cur = 0;
  std::vector<MPI_Request> reqs;
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

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | buffered_msg
      | from_node
      | cont;
  }
};

// forward-declare for header
struct MultiMsg;

/**
 * \struct ActiveMessenger active.h vt/messaging/active.h
 *
 * \brief Core component of VT used to send messages.
 *
 * ActiveMessenger is a core VT component that provides the ability to send
 * messages \c Message to registered handlers. It manages the incoming and
 * outgoing messages using MPI to communicate \c MPI_Irecv and \c MPI_Isend
 *
 * Calls that send messages, such as \c sendMsg and \c broadcastMsg,
 * relinquish ownership of the message supplied. This is implied regardless of
 * the type of the message argument. This implicit behavior is equivalent to
 * the explicit use of \c std::move.
 *
 * The following two code snippets have the same semantics:
 *
 * \code{.cpp}
 * theMsg()-sendMsg(0, msg);
 * \endcode
 *
 * \code{.cpp}
 * theMsg()-sendMsg(0, std::move(msg));
 * \endcode
 *
 * It is invalid to attempt to access the messages after such calls,
 * and doing so may result in a vtAssert or undefined behavior.
 * In special cases \c promoteMsg can be used to acquire secondary ownership.
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
  using SendFnType           = std::function<SendInfo(PtrLenPairType,NodeType,TagType)>;
  using UserSendFnType       = std::function<void(SendFnType)>;
  using ContainerPendingType = std::unordered_map<TagType,PendingRecvType>;
  using MsgContType          = std::list<BufferedMsgType>;
  using ContWaitType         = std::unordered_map<HandlerType, MsgContType>;
  using ReadyHanTagType      = std::tuple<HandlerType, TagType>;
  using MaybeReadyType       = std::vector<ReadyHanTagType>;
  using HandlerManagerType   = HandlerManager;
  using EpochStackType       = std::stack<EpochType>;
  using PendingSendType      = PendingSend;

  /**
   * \internal
   */
  ActiveMessenger();

  /**
   * \internal
   */
  virtual ~ActiveMessenger();

  std::string name() override { return "ActiveMessenger"; }

  void startup() override;

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

  trace::TraceEventIDType makeTraceCreationSend(
    HandlerType const handler, ByteType serialized_msg_size, bool is_bcast
  );

  // With serialization, the correct method is resolved via SFINAE.
  // This also includes additional guards to detect ambiguity.
  template <typename MsgT>
  ActiveMessenger::PendingSendType sendMsgSerializableImpl(
    NodeType dest,
    HandlerType han,
    MsgSharedPtr<MsgT>& msg,
    TagType tag
  );

  // All messages that do NOT define their own serialization policy
  // and do NOT define their own serialization function are required
  // to be byte-transmittable. This covers basic byte-copyable
  // messages directly inheriting from ActiveMsg. ActivMsg implements
  // a serialize function which is implictly inherited..
  template <
    typename MsgT,
    std::enable_if_t<true
      and not ::vt::messaging::msg_defines_serialize_mode<MsgT>::value,
      int
    > = 0
  >
  inline ActiveMessenger::PendingSendType sendMsgImpl(
    NodeType dest,
    HandlerType han,
    MsgSharedPtr<MsgT>& msg,
    TagType tag
  ) {
#ifndef vt_quirked_serialize_method_detection
    static_assert(
      not ::vt::messaging::has_own_serialize<MsgT>,
      "Message prohibiting serialization must not have a serialization function."
    );
#endif
    return sendMsgCopyableImpl<MsgT>(dest, han, msg, tag);
  }

  // Serializable and serialization required on this type.
  template <
    typename MsgT,
    std::enable_if_t<true
      and ::vt::messaging::msg_defines_serialize_mode<MsgT>::value
      and ::vt::messaging::msg_serialization_mode<MsgT>::required,
      int
    > = 0
  >
  inline ActiveMessenger::PendingSendType sendMsgImpl(
    NodeType dest,
    HandlerType han,
    MsgSharedPtr<MsgT>& msg,
    TagType tag
  ) {
#ifndef vt_quirked_serialize_method_detection
    static_assert(
      ::vt::messaging::has_own_serialize<MsgT>,
      "Message requiring serialization must have a serialization function."
    );
#endif
    return sendMsgSerializableImpl<MsgT>(dest, han, msg, tag);
  }

  // Serializable, but support is only for derived types.
  // This type will still be sent using byte-copy serialization.
  template <
    typename MsgT,
    std::enable_if_t<true
      and ::vt::messaging::msg_defines_serialize_mode<MsgT>::value
      and ::vt::messaging::msg_serialization_mode<MsgT>::supported,
      int
    > = 0
  >
  inline ActiveMessenger::PendingSendType sendMsgImpl(
    NodeType dest,
    HandlerType han,
    MsgSharedPtr<MsgT>& msg,
    TagType tag
  ) {
#ifndef vt_quirked_serialize_method_detection
    static_assert(
       ::vt::messaging::has_own_serialize<MsgT>,
       "Message supporting serialization must have a serialization function."
     );
#endif
    return sendMsgCopyableImpl<MsgT>(dest, han, msg, tag);
  }

  // Messaged marked as prohibiting serialization cannot define
  // a serialization function and must be sent via byte-transmission.
  template <
    typename MsgT,
    std::enable_if_t<true
      and ::vt::messaging::msg_defines_serialize_mode<MsgT>::value
      and ::vt::messaging::msg_serialization_mode<MsgT>::prohibited,
      int
    > = 0
  >
  inline ActiveMessenger::PendingSendType sendMsgImpl(
    NodeType dest,
    HandlerType han,
    MsgSharedPtr<MsgT>& msg,
    TagType tag
  ) {
#ifndef vt_quirked_serialize_method_detection
    static_assert(
      not ::vt::messaging::has_own_serialize<MsgT>,
      "Message prohibiting serialization must not have a serialization function."
    );
#endif
    return sendMsgCopyableImpl<MsgT>(dest, han, msg, tag);
  }

  template <typename MsgT>
  ActiveMessenger::PendingSendType sendMsgCopyableImpl(
    NodeType dest,
    HandlerType han,
    MsgSharedPtr<MsgT>& msg,
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
   *     theMsg()->sendMsg(29, han, msg);
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
   * \note Takes ownership of the supplied message.
   *
   * \param[in] dest node to send the message to
   * \param[in] han handler to send the message to
   * \param[in] msg the message to send
   * \param[in] msg_size the size of the message being sent
   * \param[in] tag the tag to put on the message
   *
   * \return the \c PendingSend for the send
   */
  template <typename MsgT>
  PendingSendType sendMsgSz(
    NodeType dest,
    HandlerType han,
    MsgPtrThief<MsgT> msg,
    ByteType msg_size,
    TagType tag = no_tag
  );

  /**
   * \brief Send a message with a pre-registered handler.
   *
   * \note Takes ownership of the supplied message.
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
    MsgPtrThief<MsgT> msg,
    TagType tag = no_tag
  );

  /**
   * \brief Send a message with a pre-registered handler.
   *
   * \deprecated Use \p sendMessage instead.
   *
   * \note Takes ownership of the supplied message.
   *
   * \param[in] dest the destination node to send the message to
   * \param[in] han the handler to invoke
   * \param[in] msg the message to send
   * \param[in] tag the tag to put on the message
   *
   * \return the \c PendingSend for the send
   */
  template <typename MsgT>
  PendingSendType sendMsgAuto(
    NodeType dest,
    HandlerType han,
    MsgPtrThief<MsgT> msg,
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
   * \note Takes ownership of the supplied message.
   *
   * \param[in] msg the message to broadcast
   * \param[in] msg_size the size of the message to send
   * \param[in] deliver_to_sender whether msg should be delivered to sender
   * \param[in] tag the tag to put on the message
   *
   * \return the \c PendingSend for the sent message
   */
  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  PendingSendType broadcastMsgSz(
    MsgPtrThief<MsgT> msg,
    ByteType msg_size,
    bool deliver_to_sender = true,
    TagType tag = no_tag
  );

  /**
   * \brief Broadcast a message.
   *
   * \note Takes ownership of the supplied message.
   *
   * \param[in] msg the message to broadcast
   * \param[in] deliver_to_sender whether msg should be delivered to sender
   * \param[in] tag the tag to put on the message
   *
   * \return the \c PendingSend for the sent message
   */
  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  PendingSendType broadcastMsg(
    MsgPtrThief<MsgT> msg,
    bool deliver_to_sender = true,
    TagType tag = no_tag
  );

  /**
   * \brief Send a message.
   *
   * \note Takes ownership of the supplied message.
   *
   * \param[in] dest the destination node to send the message to
   * \param[in] msg the message to send
   * \param[in] tag the tag to put on the message
   *
   * \return the \c PendingSend for the sent message
   */
  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  PendingSendType sendMsg(
    NodeType dest,
    MsgPtrThief<MsgT> msg,
    TagType tag = no_tag
  );

  /**
   * \brief Send a message with explicit size.
   *
   * Invoke this send variant if you know the size or the \c sizeof(Message) is
   * different than the number of bytes you actually want to send (e.g., extra
   * bytes on the end of the message)
   *
   * \note Takes ownership of the supplied message.
   *
   * \param[in] dest node to send the message to
   * \param[in] msg the message to send
   * \param[in] msg_size the size of the message being sent
   * \param[in] tag the tag to put on the message
   *
   * \return the \c PendingSend for the send
   */
  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  PendingSendType sendMsgSz(
    NodeType dest,
    MsgPtrThief<MsgT> msg,
    ByteType msg_size,
    TagType tag = no_tag
  );

  /**
   * \brief Broadcast a message.
   *
   * \deprecated Use \b broadcastMsg instead.
   *
   * \note Takes ownership of the supplied message.
   *
   * \param[in] msg the message to broadcast
   * \param[in] tag the tag to put on the message
   *
   * \return the \c PendingSend for the send
   */
  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  PendingSendType broadcastMsgAuto(
    MsgPtrThief<MsgT> msg,
    TagType tag = no_tag
  );

  /**
   * \brief Send a message.
   *
   * \deprecated Use \b sendMsg instead.
   *
   * \note Takes ownership of the supplied message.
   *
   * \param[in] dest the destination node to send the message to
   * \param[in] msg the message to send
   * \param[in] tag the tag to put on the message
   *
   * \return the \c PendingSend for the send
   */
  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  PendingSendType sendMsgAuto(
    NodeType dest,
    MsgPtrThief<MsgT> msg,
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
   * \note Takes ownership of the supplied message.
   *
   * \param[in] msg the message to broadcast
   * \param[in] deliver_to_sender whether msg should be delivered to sender
   * \param[in] tag the optional tag to put on the message
   *
   * \return the \c PendingSend for the broadcast
   */
  template <ActiveFnType* f, typename MsgT>
  PendingSendType broadcastMsg(
    MsgPtrThief<MsgT> msg,
    bool deliver_to_sender = true,
    TagType tag = no_tag
  );

  /**
   * \brief Send a message with a type-safe handler.
   *
   * \note Takes ownership of the supplied message.
   *
   * \param[in] dest the destination node to send the message to
   * \param[in] msg the message to broadcast
   * \param[in] tag the optional tag to put on the message
   *
   * \return the \c PendingSend for the broadcast
   */
  template <ActiveFnType* f, typename MsgT>
  PendingSendType sendMsg(
    NodeType dest,
    MsgPtrThief<MsgT> msg,
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
   * \note Takes ownership of the supplied message.
   *
   * \param[in] msg the message to broadcast
   * \param[in] deliver_to_sender whether msg should be delivered to sender
   * \param[in] tag the optional tag to put on the message
   *
   * \return the \c PendingSend for the broadcast
   */
  template <
    typename FunctorT,
    typename MsgT
  >
  PendingSendType broadcastMsg(
    MsgPtrThief<MsgT> msg,
    bool deliver_to_sender = true,
    TagType tag = no_tag
  );

  /**
   * \brief Broadcast a message.
   *
   * \note Takes ownership of the supplied message.
   *
   * \param[in] msg the message to broadcast
   * \param[in] deliver_to_sender whether msg should be delivered to sender
   * \param[in] tag the optional tag to put on the message
   *
   * \return the \c PendingSend for the broadcast
   */
  template <typename FunctorT>
  PendingSendType broadcastMsg(
    MsgPtrThief<typename util::FunctorExtractor<FunctorT>::MessageType> msg,
    bool deliver_to_sender = true,
    TagType tag = no_tag
  );

  /**
   * \brief Broadcast a message.
   *
   * \deprecated Use \p broadcastMsg instead.
   *
   * \note Takes ownership of the supplied message.
   *
   * \param[in] msg the message to send
   * \param[in] tag the optional tag to put on the message
   *
   * \return the \c PendingSend for the broadcast
   */
  template <
    typename FunctorT,
    typename MsgT = typename util::FunctorExtractor<FunctorT>::MessageType
  >
  PendingSendType broadcastMsgAuto(
    MsgPtrThief<MsgT> msg,
    TagType tag = no_tag
  );

  /**
   * \brief Send a message with a type-safe handler.
   *
   * \note Takes ownership of the supplied message.
   *
   * \param[in] dest the destination node to send the message to
   * \param[in] msg the message to broadcast
   * \param[in] tag the tag to put on the message
   *
   * \return the \c PendingSend for the send
   */
  template <typename FunctorT,typename MsgT>
  PendingSendType sendMsg(
    NodeType dest,
    MsgPtrThief<MsgT> msg,
    TagType tag = no_tag
  );

  /**
   * \brief Send a message with a type-safe handler.
   *
   * \note Takes ownership of the supplied message.
   *
   * \param[in] dest the destination node to send the message to
   * \param[in] msg the message to broadcast
   * \param[in] tag the tag to put on the message
   *
   * \return the \c PendingSend for the send
   */
  template <typename FunctorT>
  PendingSendType sendMsg(
    NodeType dest,
    MsgPtrThief<typename util::FunctorExtractor<FunctorT>::MessageType> msg,
    TagType tag = no_tag
  );

  /**
   * \brief Send a message.
   *
   * \deprecated Use \p sendMsg instead.
   *
   * \note Takes ownership of the supplied message.
   *
   * \param[in] dest the destination node to send the message to
   * \param[in] msg the message to broadcast
   * \param[in] tag the optional tag to put on the message
   *
   * \return the \c PendingSend for the send
   */
  template <
    typename FunctorT,
    typename MsgT = typename util::FunctorExtractor<FunctorT>::MessageType
  >
  PendingSendType sendMsgAuto(
    NodeType dest,
    MsgPtrThief<MsgT> msg,
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
   *       [=](PtrLenPairType ptr, ActionType deleter){
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
   *       auto ret = send(vt::PtrLenPairType{ptr, num_bytes}, put_node, vt::no_tag);
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
   * \note Takes ownership of the supplied message.
   *
   * \param[in] han the handler to invoke
   * \param[in] msg the message to broadcast
   * \param[in] deliver_to_sender whether msg should be delivered to sender
   * \param[in] tag the optional tag to put on the message
   *
   * \return the \c PendingSend for the send
   */
  template <typename MsgT>
  PendingSendType broadcastMsg(
    HandlerType han,
    MsgPtrThief<MsgT> msg,
    bool deliver_to_sender = true,
    TagType tag = no_tag
  );

  /**
   * \brief Send a message with a special payload function.
   *
   * \note Takes ownership of the supplied message.
   *
   * \param[in] dest the destination node to send the message to
   * \param[in] han the handler to invoke
   * \param[in] msg the message to send
   * \param[in] send_payload_fn
   *
   * \return the \c PendingSend for the send
   */
  template <typename MsgT>
  PendingSendType sendMsg(
    NodeType dest,
    HandlerType han,
    MsgPtrThief<MsgT> msg,
    UserSendFnType send_payload_fn
  );

  /**
   * \brief Send a message with a special payload function.
   *
   * \note Takes ownership of the supplied message.
   *
   * \param[in] dest the destination node to send the message to
   * \param[in] msg the message to send
   * \param[in] send_payload_fn
   *
   * \return the \c PendingSend for the send
   */
  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  PendingSendType sendMsg(
    NodeType dest,
    MsgPtrThief<MsgT> msg,
    UserSendFnType send_payload_fn
  );

  /**
   * \brief Broadcast a message.
   *
   * \deprecated Use \p broadcastMsg instead.
   *
   * \note Takes ownership of the supplied message.
   *
   * \param[in] han the handler to invoke
   * \param[in] msg the message to broadcast
   * \param[in] tag the optional tag to put on the message
   *
   * \return the \c PendingSend for the send
   */
  template <typename MsgT>
  PendingSendType broadcastMsgAuto(
    HandlerType han,
    MsgPtrThief<MsgT> msg,
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
   * \return information about the send for receiving the payload
   */
  SendInfo sendData(
    PtrLenPairType const& ptr, NodeType const& dest, TagType const& tag
  );

  /**
   * \internal
   * \brief Send raw bytes to a node with potentially multiple sends
   *
   * \param[in] ptr the pointer and bytes to send
   * \param[in] dest the destination node
   * \param[in] tag the MPI tag put on the send
   *
   * \return a tuple of the event and the number of sends
   */
  std::tuple<EventType, int> sendDataMPI(
    PtrLenPairType const& ptr, NodeType const& dest, TagType const& tag
  );

  /**
   * \internal
   * \brief Receive data as bytes from a node with a priority
   *
   * \param[in] nchunks the number of chunks to receive
   * \param[in] priority the priority to receive the data
   * \param[in] tag the MPI tag to receive on
   * \param[in] node the node from which to receive
   * \param[in] next a continuation to execute when data arrives
   *
   * \return whether it was successful or pending
   */
  bool recvDataMsgPriority(
    int nchunks, PriorityType priority, TagType const& tag,
    NodeType const& node, ContinuationDeleterType next = nullptr
  );

  /**
   * \internal
   * \brief Receive data as bytes from a node
   *
   * \param[in] nchunks the number of chunks to receive
   * \param[in] tag the MPI tag to receive on
   * \param[in] node the node from which to receive
   * \param[in] next a continuation to execute when data arrives
   *
   * \return whether it was successful or pending
   */
  bool recvDataMsg(
    int nchunks, TagType const& tag, NodeType const& node,
    ContinuationDeleterType next = nullptr
  );

  /**
   * \internal
   * \brief Receive data as bytes from a node with a priority
   *
   * \param[in] nchunks the number of chunks to receive
   * \param[in] priority the priority to receive the data
   * \param[in] tag the MPI tag to receive on
   * \param[in] sender the sender node
   * \param[in] enqueue whether to enqueue the pending receive
   * \param[in] next a continuation to execute when data arrives
   *
   * \return whether it was successful or pending
   */
  bool recvDataMsg(
    int nchunks, PriorityType priority, TagType const& tag,
    NodeType const& sender, bool const& enqueue,
    ContinuationDeleterType next = nullptr
  );

  /**
   * \internal
   * \brief Receive data as bytes with a buffer and priority
   *
   * \param[in] nchunks the number of chunks to receive
   * \param[in] user_buf the buffer to receive into
   * \param[in] priority the priority for the operation
   * \param[in] tag the MPI tag to use for receive
   * \param[in] node the node receiving from
   * \param[in] enqueue whether to enqueue the operation
   * \param[in] dealloc_user_buf the action to deallocate a user buffer
   * \param[in] next the continuation when the data is ready
   * \param[in] is_user_buf is a user buffer that require user deallocation
   *
   * \return whether the data is ready or pending
   */
  bool recvDataMsgBuffer(
    int nchunks, void* const user_buf, PriorityType priority, TagType const& tag,
    NodeType const& node = uninitialized_destination, bool const& enqueue = true,
    ActionType dealloc_user_buf = nullptr,
    ContinuationDeleterType next = nullptr, bool is_user_buf = false
  );

  /**
   * \internal
   * \brief Receive data as bytes with a buffer
   *
   * \param[in] nchunks the number of chunks to receive
   * \param[in] user_buf the buffer to receive into
   * \param[in] tag the MPI tag to use for receive
   * \param[in] node the node receiving from
   * \param[in] enqueue whether to enqueue the operation
   * \param[in] dealloc_user_buf the action to deallocate a user buffer
   * \param[in] next the continuation when the data is ready
   * \param[in] is_user_buf is a user buffer that require user deallocation
   *
   * \return whether the data is ready or pending
   */
  bool recvDataMsgBuffer(
    int nchunks, void* const user_buf, TagType const& tag,
    NodeType const& node = uninitialized_destination, bool const& enqueue = true,
    ActionType dealloc_user_buf = nullptr,
    ContinuationDeleterType next = nullptr, bool is_user_buf = false
  );

  /**
   * \brief Receive data from MPI in multiple chunks
   *
   * \param[in] nchunks the number of chunks to receive
   * \param[in] buf the receive buffer
   * \param[in] tag the MPI tag
   * \param[in] from the sender
   * \param[in] len the total length
   * \param[in] prio the priority for the continuation
   * \param[in] dealloc the action to deallocate the buffer
   * \param[in] next the continuation that gets passed the data when ready
   * \param[in] is_user_buf is a user buffer that require user deallocation
   */
  void recvDataDirect(
    int nchunks, void* const buf, TagType const tag, NodeType const from,
    MsgSizeType len, PriorityType prio, ActionType dealloc = nullptr,
    ContinuationDeleterType next = nullptr, bool is_user_buf = false
  );

  /**
   * \brief Receive data from MPI in multiple chunks
   *
   * \param[in] nchunks the number of chunks to receive
   * \param[in] tag the MPI tag
   * \param[in] from the sender
   * \param[in] len the total length
   * \param[in] next the continuation that gets passed the data when ready
   */
  void recvDataDirect(
    int nchunks, TagType const tag, NodeType const from,
    MsgSizeType len, ContinuationDeleterType next
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
    MsgSharedPtr<BaseMsgType>& msg
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
    HandlerType const han, ActiveClosureFnType fn, TagType const& tag = no_tag
  );

  /**
   * \internal
   * \brief Un-register a bare handler
   *
   * \param[in] han the handler to de-register
   * \param[in] tag the tag this handler will accept (\c vt::no_tag means any)
   */
  void unregisterHandlerFn(HandlerType const han, TagType const& tag = no_tag);

  /**
   * \internal
   * \brief Register a handler function for existing handler
   *
   * \param[in] han the handler to swap function pointers
   * \param[in] fn the new function pointer
   * \param[in] tag the tag this handler will accept (\c vt::no_tag means any)
   */
  void registerHandlerFn(
    HandlerType const han, ActiveClosureFnType fn, TagType const& tag = no_tag
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
   * \brief Process an incoming active message
   *
   * Forwards the message to the appropriate group nodes or broadcasts it
   * depending on envelope. May deliver the message locally or just forward it
   * depending on the target.
   *
   * \param[in] base the message ptr
   * \param[in] sender the sender of the message
   * \param[in] insert whether to insert the message if handler does not exist
   * \param[in] cont continuation after message is processed
   *
   * \return whether it was delivered locally
   */
  bool processActiveMsg(
    MsgSharedPtr<BaseMsgType> const& base, NodeType const& sender,
    bool insert, ActionType cont = nullptr
  );

  /**
   * \internal
   * \brief Prepare an active message to run by building a \c RunnableNew
   *
   * \param[in] base the message ptr
   * \param[in] from_node the node the message came from
   * \param[in] insert whether to insert the message if handler does not exist
   * \param[in] cont continuation after message is processed
   *
   * \return whether the message was delivered, false when handler does not exist
   */
  bool prepareActiveMsgToRun(
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
    HandlerType const han, TagType const& tag = no_tag
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
   * \param[in] send_tag the send tag on the message
   *
   * \return the event to test/wait for completion
   */
  EventType sendMsgBytesWithPut(
    NodeType const& dest, MsgSharedPtr<BaseMsgType> const& base,
    TagType const& send_tag
  );

  /**
   * \internal
   * \brief Send already-packed message bytes with MPI using multiple
   * sends if necessary
   *
   * \param[in] dest the destination of the message
   * \param[in] base the message base pointer
   * \param[in] msg_size the size of the message
   * \param[in] send_tag the send tag on the message
   *
   * \return the event to test/wait for completion
   */
  EventType sendMsgMPI(
    NodeType const& dest, MsgSharedPtr<BaseMsgType> const& base,
    MsgSizeType const& msg_size, TagType const& send_tag
  );

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
   * \brief Access the epoch stack
   */
  inline EpochStackType& getEpochStack() { return epoch_stack_; }

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
   * \brief Register a async operation that needs polling
   *
   * \param[in] op the async operation to register
   */
  void registerAsyncOp(std::unique_ptr<AsyncOp> op);

  /**
   * \brief Block the current task's execution on an pollable async operation
   * until it completes
   *
   * \note This may only be safely called if the current task is running in a
   * user-level thread.
   *
   * \param[in] op the async operation to block on
   */
  void blockOnAsyncOp(std::unique_ptr<AsyncOp> op);

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | maybe_ready_tag_han_
      | pending_handler_msgs_
      | pending_recvs_
      | cur_direct_buffer_tag_
      | epoch_stack_
      | in_progress_active_msg_irecv
      | in_progress_data_irecv
      | in_progress_ops
      | this_node_
      | amForwardCounterGauge
      | amHandlerCount
      | amPollCount
      | amPostedCounterGauge
      | amRecvCounterGauge
      | amSentCounterGauge
      | bcastsSentCount
      | dmPollCount
      | dmPostedCounterGauge
      | dmRecvCounterGauge
      | dmSentCounterGauge
      | tdRecvCount
      | tdSentCount;

  # if vt_check_enabled(trace_enabled)
    s | trace_irecv
      | trace_isend
      | trace_irecv_polling_am
      | trace_irecv_polling_dm
      | trace_asyncop;
  # endif
  }

private:
  /**
   * \internal \brief Allocate a new, unused tag.
   *
   * \note Wraps around when reaching max tag, determined by the MPI
   * implementation.
   *
   * \return a new MPI tag
   */
  MPI_TagType allocateNewTag();

  /**
   * \internal \brief Handle a control message that coordinates multiple
   * payloads arriving that constitute a contiguous payload
   *
   * \param[in] msg the message with control data
   */
  void handleChunkedMultiMsg(MultiMsg* msg);

  /**
   * \internal \brief Handle a control message; immediately calls
   * \c handleChunkedMultiMsg
   *
   * \param[in] msg the message with control data
   */
  static void chunkedMultiMsg(MultiMsg* msg);

  /**
   * \brief Test pending MPI request for active message receives
   *
   * \return whether progress was made
   */
  bool testPendingActiveMsgAsyncRecv();

  /**
   * \brief Test pending MPI request for data message receives
   *
   * \return whether progress was made
   */
  bool testPendingDataMsgAsyncRecv();

  /**
   * \brief Test pending general asynchronous events
   *
   * \return whether progress was made
   */
  bool testPendingAsyncOps();

  /**
   * \brief Called when a VT-MPI message has been received.
   */
  void finishPendingActiveMsgAsyncRecv(InProgressIRecv* irecv);

  /**
   * \brief Called when a VT-MPI message has been received.
   */
  void finishPendingDataMsgAsyncRecv(InProgressDataIRecv* irecv);

  /**
   * @brief Record LB's statistics for sending a message
   *
   * \param[in] dest the destination of the message
   * \param[in] base the message base pointer
   * \param[in] msg_size the size of the message being sent
   */
  void recordLbStatsCommForSend(
    NodeType const dest, MsgSharedPtr<BaseMsgType> const& base,
    MsgSizeType const msg_size
  );

private:
# if vt_check_enabled(trace_enabled)
  trace::UserEventIDType trace_irecv             = trace::no_user_event_id;
  trace::UserEventIDType trace_isend             = trace::no_user_event_id;
  trace::UserEventIDType trace_irecv_polling_am  = trace::no_user_event_id;
  trace::UserEventIDType trace_irecv_polling_dm  = trace::no_user_event_id;
  trace::UserEventIDType trace_asyncop           = trace::no_user_event_id;
# endif

  MaybeReadyType maybe_ready_tag_han_                     = {};
  ContWaitType pending_handler_msgs_                      = {};
  ContainerPendingType pending_recvs_                     = {};
  TagType cur_direct_buffer_tag_                          = starting_direct_buffer_tag;
  EpochStackType epoch_stack_;
  RequestHolder<InProgressIRecv> in_progress_active_msg_irecv;
  RequestHolder<InProgressDataIRecv> in_progress_data_irecv;
  RequestHolder<AsyncOpWrapper> in_progress_ops;
  NodeType this_node_                                     = uninitialized_destination;

private:
  // Diagnostic counter gauge combos for sent counts/bytes
  diagnostic::CounterGauge amSentCounterGauge;
  diagnostic::CounterGauge dmSentCounterGauge;

  // Diagnostic counters for recv counts/bytes
  diagnostic::CounterGauge amRecvCounterGauge;
  diagnostic::CounterGauge dmRecvCounterGauge;

  // Diagnostic counters for posted irecv counts/bytes
  diagnostic::CounterGauge amPostedCounterGauge;
  diagnostic::CounterGauge dmPostedCounterGauge;

  // Diagnostic counters for counting various actions
  diagnostic::Counter amHandlerCount;
  diagnostic::Counter bcastsSentCount;
  diagnostic::Counter amPollCount;
  diagnostic::Counter dmPollCount;
  diagnostic::Counter tdSentCount;
  diagnostic::Counter tdRecvCount;

  // Diagnostic counters for counting forwarded messages
  diagnostic::CounterGauge amForwardCounterGauge;

private:
  elm::ElementIDStruct bare_handler_dummy_elm_id_for_lb_stats_ = {};
  elm::ElementStats bare_handler_stats_;
};

}} // end namespace vt::messaging

namespace vt {

using Active = messaging::ActiveMessenger;

extern messaging::ActiveMessenger* theMsg();

} // end namespace vt

#include "vt/messaging/active.impl.h"

#endif /*INCLUDED_VT_MESSAGING_ACTIVE_H*/
