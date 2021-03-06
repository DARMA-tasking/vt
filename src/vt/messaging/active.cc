/*
//@HEADER
// *****************************************************************************
//
//                                  active.cc
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


#include "vt/config.h"
#include "vt/configs/arguments/app_config.h"
#include "vt/messaging/active.h"
#include "vt/messaging/envelope.h"
#include "vt/messaging/message/smart_ptr.h"
#include "vt/termination/term_headers.h"
#include "vt/group/group_manager_active_attorney.h"
#include "vt/runnable/general.h"
#include "vt/timing/timing.h"
#include "vt/scheduler/priority.h"
#include "vt/utils/mpi_limits/mpi_max_tag.h"
#include "vt/runtime/mpi_access.h"
#include "vt/scheduler/scheduler.h"

namespace vt { namespace messaging {

ActiveMessenger::ActiveMessenger()
  :
# if vt_check_enabled(trace_enabled)
  trace_irecv(trace::registerEventCollective("MPI_Irecv")),
  trace_isend(trace::registerEventCollective("MPI_Isend")),
  trace_irecv_polling_am(trace::registerEventCollective("IRecv: Active Msg poll")),
  trace_irecv_polling_dm(trace::registerEventCollective("IRecv: Data Msg poll")),
  trace_asyncop(trace::registerEventCollective("AsyncOP: poll")),
  in_progress_active_msg_irecv(trace_irecv_polling_am),
  in_progress_data_irecv(trace_irecv_polling_dm),
  in_progress_ops(trace_asyncop),
# endif
  this_node_(theContext()->getNode())
{
  /*
   * Push the default epoch into the stack so it is always at the bottom of the
   * stack during execution until the AM's destructor is invoked
   */
  pushEpoch(term::any_epoch_sentinel);

  // Register counters for AM/DM message sends and number of bytes
  amSentCounterGauge = diagnostic::CounterGauge{
    registerCounter("AM_sent", "active messages sent"),
    registerGauge("AM_sent_bytes", "active messages bytes sent", UnitType::Bytes)
  };

  dmSentCounterGauge = diagnostic::CounterGauge{
    registerCounter("DM_sent", "data messages sent"),
    registerGauge("DM_sent_bytes", "data messages bytes sent", UnitType::Bytes)
  };

  // Register counters for AM/DM message receives and number of bytes
  amRecvCounterGauge = diagnostic::CounterGauge{
    registerCounter("AM_recv", "active messages received"),
    registerGauge(
      "AM_recv_bytes", "active message bytes received", UnitType::Bytes
    )
  };

  dmRecvCounterGauge = diagnostic::CounterGauge{
    registerCounter("DM_recv", "data messages received"),
    registerGauge(
      "DM_recv_bytes", "data message bytes received", UnitType::Bytes
    )
  };

  // Register counters for AM/DM message MPI_Irecv posts This is useful as a
  // debugging diagnostic if the program hangs. Checking this against AM_recv,
  // etc will inform whether if there are outstanding posted receives
  amPostedCounterGauge = diagnostic::CounterGauge{
    registerCounter("AM_recv_posted", "active message irecvs posted"),
    registerGauge(
      "AM_recv_posted_bytes", "active message irecv posted bytes", UnitType::Bytes
    )
  };

  dmPostedCounterGauge = diagnostic::CounterGauge{
    registerCounter("DM_recv_posted", "data message irecvs posted"),
    registerGauge(
      "DM_recv_posted_bytes", "data message irecv posted bytes", UnitType::Bytes
    )
  };

  // Number of AM handlers executed
  amHandlerCount = registerCounter(
    "AM_handlers", "active message handlers"
  );

  // Number of broadcast messages that this node sent
  bcastsSentCount = registerCounter(
    "bcasts_sent", "active message broadcasts sent"
  );

  // Number of MPI_Test polls for AM/DM
  amPollCount = registerCounter("AM_polls", "active message polls");
  dmPollCount = registerCounter("DM_polls", "data message polls");

  // Number of termination message sent/received
  tdSentCount = registerCounter("TD_sent", "termination messages sent");
  tdRecvCount = registerCounter("TD_recv", "termination messages recv");

  // Number of messages that were purely forwarded to another node by this AM
  amForwardCounterGauge = diagnostic::CounterGauge{
    registerCounter("AM_forwarded", "messages forwarded (and not delivered)"),
    registerGauge(
      "AM_forwarded_bytes", "messages forwarded (and not delivered)",
      UnitType::Bytes
    )
  };
}

/*virtual*/ ActiveMessenger::~ActiveMessenger() {
  // Pop all extraneous epochs off the stack greater than 1
  auto stack_size = epoch_stack_.size();
  while (stack_size > 1) {
    stack_size = (epoch_stack_.pop(), epoch_stack_.size());
  }
  // Pop off the last epoch: term::any_epoch_sentinel
  auto const ret_epoch = popEpoch(term::any_epoch_sentinel);
  vtAssertInfo(
    ret_epoch == term::any_epoch_sentinel, "Last pop must be any epoch",
    ret_epoch, term::any_epoch_sentinel, epoch_stack_.size()
  );
  vtAssertExpr(epoch_stack_.size() == 0);
}

void ActiveMessenger::packMsg(
  MessageType* msg, MsgSizeType size, void* ptr, MsgSizeType ptr_bytes
) {
  vt_debug_print(
    active, node,
    "packMsg: msg_size={}, put_size={}, ptr={}\n",
    size, ptr_bytes, print_ptr(ptr)
  );

  char* const msg_buffer = reinterpret_cast<char*>(msg) + size;
  std::memcpy(msg_buffer, ptr, ptr_bytes);
}

EventType ActiveMessenger::sendMsgBytesWithPut(
  NodeType const& dest, MsgSharedPtr<BaseMsgType> const& base,
  MsgSizeType const& msg_size, TagType const& send_tag
) {
  auto msg = base.get();
  auto const& is_term = envelopeIsTerm(msg->env);
  auto const& is_put = envelopeIsPut(msg->env);
  auto const& is_put_packed = envelopeIsPackedPutType(msg->env);
  auto const& is_bcast = envelopeIsBcast(msg->env);

  if (!is_term || vt_check_enabled(print_term_msgs)) {
    vt_debug_print(
      active, node,
      "sendMsgBytesWithPut: size={}, dest={}, is_put={}, is_put_packed={}\n",
      msg_size, dest, print_bool(is_put), print_bool(is_put_packed)
    );
  }

  vtWarnIf(
    !(dest != theContext()->getNode() || is_bcast),
    "Destination {} should != this node"
  );

  MsgSizeType new_msg_size = msg_size;

  if (is_put && !is_put_packed) {
    auto const& put_ptr = envelopeGetPutPtr(msg->env);
    auto const& put_size = envelopeGetPutSize(msg->env);
    bool const& memory_pool_active = thePool()->active_env();
    auto const& rem_size = thePool()->remainingSize(msg);
    /*
     * Directly pack if the pool is active (which means it may have
     * overallocated and the remaining size of the (envelope) buffer is
     * sufficient for the for the put payload.
     */
    bool const direct_buf_pack =
      memory_pool_active                 &&
      put_size <= rem_size               &&
      put_size <= max_pack_direct_size;
    vtAssert(
      (!(put_size != 0) || put_ptr), "Must have valid ptr if size > 0"
    );

    if (!is_term || vt_check_enabled(print_term_msgs)) {
      vt_debug_print(
        active, node,
        "sendMsgBytesWithPut: (put) put_ptr={}, size:[msg={},put={},rem={}],"
        "dest={}, max_pack_size={}, direct_buf_pack={}\n",
        put_ptr, msg_size, put_size, rem_size, dest, max_pack_direct_size,
        print_bool(direct_buf_pack)
      );
    }
    if (direct_buf_pack) {
      packMsg(msg, msg_size, put_ptr, put_size);
      new_msg_size += put_size;
      envelopeSetPutTag(msg->env, PutPackedTag);
      setPackedPutType(msg->env);
    } else {
      auto const& env_tag = envelopeGetPutTag(msg->env);
      auto const& ret = sendData(
        PtrLenPairType{put_ptr,put_size}, dest, env_tag
      );
      auto const& ret_tag = ret.getTag();
      if (ret_tag != env_tag) {
        envelopeSetPutTag(msg->env, ret_tag);
      }
    }
  } else if (is_put && is_put_packed) {
    // Adjust size of the send for packed data
    auto const& put_size = envelopeGetPutSize(msg->env);
    new_msg_size += put_size;
  }

  sendMsgBytes(dest, base, new_msg_size, send_tag);

  return no_event;
}

struct MultiMsg : vt::Message {
  MultiMsg(SendInfo in_info, NodeType in_from, MsgSizeType in_size)
    : info(in_info),
      from(in_from),
      size(in_size)
  { }

  SendInfo getInfo() const { return info; }
  NodeType getFrom() const { return from; }
  MsgSizeType getSize() const { return size; }

private:
  SendInfo info;
  NodeType from = uninitialized_destination;
  MsgSizeType size = 0;
};

/*static*/ void ActiveMessenger::chunkedMultiMsg(MultiMsg* msg) {
  theMsg()->handleChunkedMultiMsg(msg);
}

void ActiveMessenger::handleChunkedMultiMsg(MultiMsg* msg) {
  auto buf =
#if vt_check_enabled(memory_pool)
    static_cast<char*>(thePool()->alloc(msg->getSize()));
#else
    static_cast<char*>(std::malloc(msg->getSize()));
#endif

  auto const size = msg->getSize();
  auto const info = msg->getInfo();
  auto const sender = msg->getFrom();
  auto const nchunks = info.getNumChunks();
  auto const tag = info.getTag();

  auto fn = [buf,sender,size,tag,this](PtrLenPairType,ActionType){
    vt_debug_print(
      active, node,
      "handleChunkedMultiMsg: all chunks arrived tag={}, size={}, from={}\n",
      tag, size, sender
    );
    InProgressIRecv irecv(buf, size, sender);
    finishPendingActiveMsgAsyncRecv(&irecv);
  };

  recvDataDirect(nchunks, buf, tag, sender, size, 0, nullptr, fn, false);
}

EventType ActiveMessenger::sendMsgMPI(
  NodeType const& dest, MsgSharedPtr<BaseMsgType> const& base,
  MsgSizeType const& msg_size, TagType const& send_tag
) {
  BaseMsgType* base_typed_msg = base.get();

  char* untyped_msg = reinterpret_cast<char*>(base_typed_msg);

  vt_debug_print(
    active, node,
    "sendMsgMPI: dest={}, msg_size={}, send_tag={}\n",
    dest, msg_size, send_tag
  );

  auto const max_per_send = theConfig()->vt_max_mpi_send_size;
  if (static_cast<std::size_t>(msg_size) < max_per_send) {
    auto const event_id = theEvent()->createMPIEvent(this_node_);
    auto& holder = theEvent()->getEventHolder(event_id);
    auto mpi_event = holder.get_event();

    mpi_event->setManagedMessage(base.to<ShortMessage>());

    int small_msg_size = static_cast<int>(msg_size);
    {
      VT_ALLOW_MPI_CALLS;
      #if vt_check_enabled(trace_enabled)
        double tr_begin = 0;
        if (theConfig()->vt_trace_mpi) {
          tr_begin = vt::timing::Timing::getCurrentTime();
        }
      #endif
      int const ret = MPI_Isend(
        untyped_msg, small_msg_size, MPI_BYTE, dest, send_tag,
        theContext()->getComm(), mpi_event->getRequest()
      );
      vtAssertMPISuccess(ret, "MPI_Isend");

      #if vt_check_enabled(trace_enabled)
        if (theConfig()->vt_trace_mpi) {
          auto tr_end = vt::timing::Timing::getCurrentTime();
          auto tr_note = fmt::format("Isend(AM): dest={}, bytes={}", dest, msg_size);
          trace::addUserBracketedNote(tr_begin, tr_end, tr_note, trace_isend);
        }
      #endif
    }

    return event_id;
  } else {
    vt_debug_print(
      active, node,
      "sendMsgMPI: (multi): size={}\n", msg_size
    );
    auto tag = allocateNewTag();
    auto this_node = theContext()->getNode();

    // Send the actual data in multiple chunks
    PtrLenPairType tup = std::make_tuple(untyped_msg, msg_size);
    SendInfo info = sendData(tup, dest, tag);

    auto event_id = info.getEvent();
    auto& holder = theEvent()->getEventHolder(event_id);
    auto mpi_event = holder.get_event();
    mpi_event->setManagedMessage(base.to<ShortMessage>());

    // Send the control message to receive the multiple chunks of data
    auto m = makeMessage<MultiMsg>(info, this_node, msg_size);
    sendMsg<MultiMsg, chunkedMultiMsg>(dest, m);

    return event_id;
  }
}

EventType ActiveMessenger::sendMsgBytes(
  NodeType const& dest, MsgSharedPtr<BaseMsgType> const& base,
  MsgSizeType const& msg_size, TagType const& send_tag
) {
  auto const& msg = base.get();

  auto const epoch = envelopeIsEpochType(msg->env) ?
    envelopeGetEpoch(msg->env) : term::any_epoch_sentinel;
  auto const is_term = envelopeIsTerm(msg->env);
  auto const is_bcast = envelopeIsBcast(msg->env);

  if (!is_term || vt_check_enabled(print_term_msgs)) {
    vt_debug_print(
      active, node,
      "sendMsgBytes: size={}, dest={}\n", msg_size, dest
    );
  }

  vtAbortIf(
    dest >= theContext()->getNumNodes() || dest < 0, "Invalid destination: {}"
  );

  if (is_bcast) {
    bcastsSentCount.increment(1);
  }
  if (is_term) {
    tdSentCount.increment(1);
  }
  amSentCounterGauge.incrementUpdate(msg_size, 1);

  EventType const event_id = sendMsgMPI(dest, base, msg_size, send_tag);

  if (not is_term) {
    theTerm()->produce(epoch,1,dest);
    theTerm()->hangDetectSend();
  }

  for (auto&& l : send_listen_) {
    l->send(dest, msg_size, is_bcast);
  }

  return event_id;
}

#if vt_check_enabled(trace_enabled)
trace::TraceEventIDType ActiveMessenger::getCurrentTraceEvent() const {
  return current_trace_context_;
}
#endif

EventType ActiveMessenger::doMessageSend(
  MsgSharedPtr<BaseMsgType>& base, MsgSizeType msg_size
) {
  auto const& send_tag = static_cast<MPI_TagType>(MPITag::ActiveMsgTag);

  auto msg = base.get();

  auto const dest = envelopeGetDest(msg->env);
  auto const is_bcast = envelopeIsBcast(msg->env);
  auto const is_term = envelopeIsTerm(msg->env);

  #if vt_check_enabled(trace_enabled)
    envelopeSetIsLocked(msg->env, false);

    // We are not allowed to hold a ref to anything in the envelope, get this,
    // modify it and put it back
    auto handler = envelopeGetHandler(msg->env);

    // If trace_rt_enabled is set locally on the envelope (which was set when
    // the message was created before the handler was setup based on
    // configuration), we need to set the trace bits on the handler. This
    // enables the handler-inside-handler to correctly enable tracing on a very
    // specific part of the execution, configured by the user (e.g., location
    // tracing enabled, termination tracing disabled)
    HandlerManagerType::setHandlerTrace(
      handler, envelopeGetTraceRuntimeEnabled(msg->env)
    );

    // Update the handler with trace bits now set appropriately
    envelopeSetHandler(msg->env, handler);

    if (not is_bcast or (is_bcast and dest == this_node_)) {
      // auto cur_event = envelopeGetTraceEvent(msg->env);
      // if (cur_event == trace::no_trace_event) {
      auto event = makeTraceCreationSend(
        base, handler, auto_registry::RegistryTypeEnum::RegGeneral,
        msg_size, is_bcast
      );
      envelopeSetTraceEvent(msg->env, event);
    }

    envelopeSetIsLocked(msg->env, true);
  #endif

  if (!is_term || vt_check_enabled(print_term_msgs)) {
    vt_debug_print(
      active, node,
      "doMessageSend: dest={}, handler={:x}, is_bcast={}, is_put={}\n",
      dest, envelopeGetHandler(msg->env), print_bool(is_bcast),
      print_bool(envelopeIsPut(msg->env))
    );
  }

  bool deliver = false;
  EventType const ret_event = group::GroupActiveAttorney::groupHandler(
    base, uninitialized_destination, msg_size, true, &deliver
  );

  if (deliver) {
    sendMsgBytesWithPut(dest, base, msg_size, send_tag);
    return no_event;
  }

  return ret_event;
}

MPI_TagType ActiveMessenger::allocateNewTag() {
  auto const max_tag = util::MPI_Attr::getMaxTag();

  if (cur_direct_buffer_tag_ == max_tag) {
    cur_direct_buffer_tag_ = starting_direct_buffer_tag;
  }
  auto const ret_tag = cur_direct_buffer_tag_++;

  return ret_tag;
}

SendInfo ActiveMessenger::sendData(
  PtrLenPairType const& ptr, NodeType const& dest, TagType const& tag
) {
  auto const& data_ptr = std::get<0>(ptr);
  auto const& num_bytes = std::get<1>(ptr);

  int send_tag = 0;
  if (tag != no_tag) {
    send_tag = tag;
  } else {
    send_tag = allocateNewTag();
  }

  vt_debug_print(
    active, node,
    "sendData: ptr={}, num_bytes={} dest={}, tag={}, send_tag={}\n",
    data_ptr, num_bytes, dest, tag, send_tag
  );

  vtAbortIf(
    dest >= theContext()->getNumNodes() || dest < 0,
    "Invalid destination: {}"
  );

  dmSentCounterGauge.incrementUpdate(num_bytes, 1);

  auto ret = sendDataMPI(ptr, dest, send_tag);
  EventType event_id = std::get<0>(ret);
  int num = std::get<1>(ret);

  // Assume that any raw data send/recv is paired with a message with an epoch
  // if required to inhibit early termination of that epoch
  theTerm()->produce(term::any_epoch_sentinel,1,dest);
  theTerm()->hangDetectSend();

  for (auto&& l : send_listen_) {
    l->send(dest, num_bytes, false);
  }

  return SendInfo{event_id, send_tag, num};
}

std::tuple<EventType, int> ActiveMessenger::sendDataMPI(
  PtrLenPairType const& payload, NodeType const& dest, TagType const& tag
) {
  auto ptr = static_cast<char*>(std::get<0>(payload));
  auto remainder = std::get<1>(payload);
  int num_sends = 0;
  std::vector<EventType> events;
  EventType ret_event = no_event;
  auto const max_per_send = theConfig()->vt_max_mpi_send_size;
  while (remainder > 0) {
    auto const event_id = theEvent()->createMPIEvent(this_node_);
    auto& holder = theEvent()->getEventHolder(event_id);
    auto mpi_event = holder.get_event();
    auto subsize = static_cast<ByteType>(
      std::min(static_cast<std::size_t>(remainder), max_per_send)
    );
    {
      #if vt_check_enabled(trace_enabled)
        double tr_begin = 0;
        if (theConfig()->vt_trace_mpi) {
          tr_begin = vt::timing::Timing::getCurrentTime();
        }
      #endif

      vt_debug_print(
        active, node,
        "sendDataMPI: remainder={}, node={}, tag={}, num_sends={}, subsize={},"
        "total size={}\n",
        remainder, dest, tag, num_sends, subsize, std::get<1>(payload)
      );

      VT_ALLOW_MPI_CALLS;
      int const ret = MPI_Isend(
        ptr, subsize, MPI_BYTE, dest, tag, theContext()->getComm(),
        mpi_event->getRequest()
      );
      vtAssertMPISuccess(ret, "MPI_Isend");

      #if vt_check_enabled(trace_enabled)
        if (theConfig()->vt_trace_mpi) {
          auto tr_end = vt::timing::Timing::getCurrentTime();
          auto tr_note = fmt::format("Isend(Data): dest={}, bytes={}", dest, subsize);
          trace::addUserBracketedNote(tr_begin, tr_end, tr_note, trace_isend);
        }
      #endif
    }
    ptr += subsize;
    remainder -= subsize;
    num_sends++;
    events.push_back(event_id);
  }

  if (events.size() > 1) {
    ret_event = theEvent()->createParentEvent(theContext()->getNode());
    auto& holder = theEvent()->getEventHolder(ret_event);
    for (auto&& child_event : events) {
      holder.get_event()->addEventToList(child_event);
    }
  } else {
    vtAssert(events.size() > 0, "Must contain at least one event");
    ret_event = events.back();
  }

  return std::make_tuple(ret_event, num_sends);
}

bool ActiveMessenger::recvDataMsgPriority(
  int nchunks, PriorityType priority, TagType const& tag, NodeType const& node,
  ContinuationDeleterType next
) {
  return recvDataMsg(nchunks, priority, tag, node, true, next);
}

bool ActiveMessenger::recvDataMsg(
  int nchunks, TagType const& tag, NodeType const& node,
  ContinuationDeleterType next
) {
  return recvDataMsg(nchunks, default_priority, tag, node, true, next);
}

bool ActiveMessenger::tryProcessDataMsgRecv() {
  bool erase = false;
  auto iter = pending_recvs_.begin();

  for (; iter != pending_recvs_.end(); ++iter) {
    auto& elm = iter->second;
    auto const done = recvDataMsgBuffer(
      elm.nchunks, elm.user_buf, elm.priority, iter->first, elm.sender, false,
      elm.dealloc_user_buf, elm.cont, elm.is_user_buf
    );
    if (done) {
      erase = true;
      break;
    }
  }

  if (erase) {
    pending_recvs_.erase(iter);
    return true;
  } else {
    return false;
  }
}

bool ActiveMessenger::recvDataMsgBuffer(
  int nchunks, void* const user_buf, TagType const& tag,
  NodeType const& node, bool const& enqueue, ActionType dealloc,
  ContinuationDeleterType next, bool is_user_buf
) {
  return recvDataMsgBuffer(
    nchunks, user_buf, no_priority, tag, node, enqueue, dealloc, next,
    is_user_buf
  );
}

bool ActiveMessenger::recvDataMsgBuffer(
  int nchunks, void* const user_buf, PriorityType priority, TagType const& tag,
  NodeType const& node, bool const& enqueue, ActionType dealloc_user_buf,
  ContinuationDeleterType next, bool is_user_buf
) {
  if (not enqueue) {
    CountType num_probe_bytes;
    MPI_Status stat;
    int flag;

    {
      VT_ALLOW_MPI_CALLS;
      const int probe_ret = MPI_Iprobe(
        node == uninitialized_destination ? MPI_ANY_SOURCE : node,
        tag, theContext()->getComm(), &flag, &stat
      );
      vtAssertMPISuccess(probe_ret, "MPI_Iprobe");
    }

    if (flag == 1) {
      MPI_Get_count(&stat, MPI_BYTE, &num_probe_bytes);

      char* buf =
        user_buf == nullptr ?

    #if vt_check_enabled(memory_pool)
        static_cast<char*>(thePool()->alloc(num_probe_bytes)) :
    #else
        static_cast<char*>(std::malloc(num_probe_bytes))      :
    #endif

        static_cast<char*>(user_buf);

      NodeType const sender = stat.MPI_SOURCE;

      recvDataDirect(
        nchunks, buf, tag, sender, num_probe_bytes, priority, dealloc_user_buf,
        next, is_user_buf
      );

      return true;
    } else {
      return false;
    }
  } else {
    vt_debug_print(
      active, node,
      "recvDataMsgBuffer: nchunks={}, node={}, tag={}, enqueue={}, "
      "priority={:x} buffering, is_user_buf={}\n",
      nchunks, node, tag, print_bool(enqueue), priority, is_user_buf
    );

    pending_recvs_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(tag),
      std::forward_as_tuple(
        PendingRecvType{
          nchunks, user_buf, next, dealloc_user_buf, node, priority,
          is_user_buf
        }
      )
    );
    return false;
  }
}

void ActiveMessenger::recvDataDirect(
  int nchunks, TagType const tag, NodeType const from, MsgSizeType len,
  ContinuationDeleterType next
) {
  char* buf =
    #if vt_check_enabled(memory_pool)
      static_cast<char*>(thePool()->alloc(len));
    #else
      static_cast<char*>(std::malloc(len));
    #endif

  recvDataDirect(
    nchunks, buf, tag, from, len, default_priority, nullptr, next, false
  );
}

void ActiveMessenger::recvDataDirect(
  int nchunks, void* const buf, TagType const tag, NodeType const from,
  MsgSizeType len, PriorityType prio, ActionType dealloc,
  ContinuationDeleterType next, bool is_user_buf
) {
  vtAssert(nchunks > 0, "Must have at least one chunk");

  std::vector<MPI_Request> reqs;
  reqs.resize(nchunks);

  char* cbuf = static_cast<char*>(buf);
  MsgSizeType remainder = len;
  auto const max_per_send = theConfig()->vt_max_mpi_send_size;
  for (int i = 0; i < nchunks; i++) {
    auto sublen = static_cast<int>(
      std::min(static_cast<std::size_t>(remainder), max_per_send)
    );

    #if vt_check_enabled(trace_enabled)
      double tr_begin = 0;
      if (theConfig()->vt_trace_mpi) {
        tr_begin = vt::timing::Timing::getCurrentTime();
      }
    #endif

    {
      VT_ALLOW_MPI_CALLS;
      int const ret = MPI_Irecv(
        cbuf+(i*max_per_send), sublen, MPI_BYTE, from, tag,
        theContext()->getComm(), &reqs[i]
      );
      vtAssertMPISuccess(ret, "MPI_Irecv");
    }

    dmPostedCounterGauge.incrementUpdate(len, 1);

    #if vt_check_enabled(trace_enabled)
      if (theConfig()->vt_trace_mpi) {
        auto tr_end = vt::timing::Timing::getCurrentTime();
        auto tr_note = fmt::format(
          "Irecv(Data): from={}, bytes={}",
          from, sublen
        );
        trace::addUserBracketedNote(tr_begin, tr_end, tr_note, trace_irecv);
      }
    #endif

    remainder -= sublen;
  }

  InProgressDataIRecv recv{
    cbuf, len, from, std::move(reqs), is_user_buf ? buf : nullptr, dealloc,
    next, prio
  };

  int num_mpi_tests = 0;
  bool done = recv.test(num_mpi_tests);

  dmPollCount.increment(num_mpi_tests);

  if (done) {
    finishPendingDataMsgAsyncRecv(&recv);
  } else {
    in_progress_data_irecv.emplace(std::move(recv));
  }
}

void ActiveMessenger::finishPendingDataMsgAsyncRecv(InProgressDataIRecv* irecv) {
  auto buf = irecv->buf;
  auto num_probe_bytes = irecv->probe_bytes;
  auto sender = irecv->sender;
  auto user_buf = irecv->user_buf;
  auto dealloc_user_buf = irecv->dealloc_user_buf;
  auto next = irecv->next;

# if vt_check_enabled(trace_enabled)
  if (theConfig()->vt_trace_mpi) {
    auto tr_note = fmt::format("DM Irecv completed: from={}", irecv->sender);
    trace::addUserNote(tr_note);
  }
# endif

  dmRecvCounterGauge.incrementUpdate(num_probe_bytes, 1);

  auto dealloc_buf = [=]{
    vt_debug_print(
      active, node,
      "finishPendingDataMsgAsyncRecv: continuation user_buf={}, buf={}\n",
      user_buf, buf
    );

    if (user_buf == nullptr) {
      #if vt_check_enabled(memory_pool)
        thePool()->dealloc(buf);
      #else
        std::free(buf);
      #endif
    } else if (dealloc_user_buf != nullptr and user_buf != nullptr) {
      dealloc_user_buf();
    }
  };

  if (next == nullptr) {
    dealloc_buf();
    theTerm()->consume(term::any_epoch_sentinel,1,sender);
    theTerm()->hangDetectRecv();
  } else {
    // If we have a continuation, schedule to run later
    auto run = [=]{
      next(PtrLenPairType{buf,num_probe_bytes}, dealloc_buf);
      theTerm()->consume(term::any_epoch_sentinel,1,sender);
      theTerm()->hangDetectRecv();
    };
    theSched()->enqueue(irecv->priority, run);
  }
}

bool ActiveMessenger::recvDataMsg(
  int nchunks, PriorityType priority, TagType const& tag,
  NodeType const& sender, bool const& enqueue,
  ContinuationDeleterType next
) {
  return recvDataMsgBuffer(
    nchunks, nullptr, priority, tag, sender, enqueue, nullptr, next
  );
}

NodeType ActiveMessenger::getFromNodeCurrentHandler() const {
  return current_node_context_;
}

void ActiveMessenger::scheduleActiveMsg(
  MsgSharedPtr<BaseMsgType> const& base, NodeType const& from,
  MsgSizeType const& size, bool insert, ActionType cont
) {
  vt_debug_print_verbose(
    active, node,
    "scheduleActiveMsg: msg={}, from={}, size={}, insert={}\n",
    print_ptr(base.get()), from, size, insert
  );

  // Enqueue the message for processing
  auto run = [=]{ processActiveMsg(base, from, size, insert, cont); };
  theSched()->enqueue(base, run);
}

bool ActiveMessenger::processActiveMsg(
  MsgSharedPtr<BaseMsgType> const& base, NodeType const& from,
  MsgSizeType const& size, bool insert, ActionType cont
) {
  using ::vt::group::GroupActiveAttorney;

  auto msg = base.to<ShortMessage>().get();

  // Call group handler
  bool deliver = false;
  GroupActiveAttorney::groupHandler(base, from, size, false, &deliver);

  auto const is_term = envelopeIsTerm(msg->env);

  if (!is_term || vt_check_enabled(print_term_msgs)) {
    vt_debug_print(
      active, node,
      "processActiveMsg: msg={}, ref={}, deliver={}\n",
      print_ptr(msg), envelopeGetRef(msg->env), print_bool(deliver)
    );
  }

  if (deliver) {
    return deliverActiveMsg(base,from,insert,cont);
  } else {
    amForwardCounterGauge.incrementUpdate(size, 1);

    if (cont != nullptr) {
      cont();
    }
    return false;
  }
}

bool ActiveMessenger::deliverActiveMsg(
  MsgSharedPtr<BaseMsgType> const& base, NodeType const& in_from_node,
  bool insert, ActionType cont
) {
  using MsgType = ShortMessage;
  auto msg = base.to<MsgType>().get();

  auto const is_term = envelopeIsTerm(msg->env);
  auto const is_bcast = envelopeIsBcast(msg->env);
  auto const dest = envelopeGetDest(msg->env);
  auto const handler = envelopeGetHandler(msg->env);
  auto const epoch = envelopeIsEpochType(msg->env) ?
    envelopeGetEpoch(msg->env) : term::any_epoch_sentinel;
  auto const is_tag = envelopeIsTagType(msg->env);
  auto const tag = is_tag ? envelopeGetTag(msg->env) : no_tag;
  auto const from_node = is_bcast ? dest : in_from_node;

  ActiveFnPtrType active_fun = nullptr;

  bool has_ex_handler = false;
  bool const is_auto = HandlerManagerType::isHandlerAuto(handler);
  bool const is_functor = HandlerManagerType::isHandlerFunctor(handler);
  bool const is_obj = HandlerManagerType::isHandlerObjGroup(handler);

  if (!is_term || vt_check_enabled(print_term_msgs)) {
    vt_debug_print(
      active, node,
      "deliverActiveMsg: msg={}, ref={}, is_bcast={}, epoch={:x}\n",
      print_ptr(msg), envelopeGetRef(msg->env), print_bool(is_bcast),
      epoch
    );
  }

  if (is_auto and is_functor and not is_obj) {
    active_fun = auto_registry::getAutoHandlerFunctor(handler);
  } else if (is_auto and not is_obj) {
    active_fun = auto_registry::getAutoHandler(handler);
  } else if (not is_obj) {
    auto ret = theRegistry()->getHandler(handler, tag);
    if (ret != nullptr) {
      has_ex_handler = true;
    }
  }

  bool const has_handler = active_fun != no_action or has_ex_handler or is_obj;

  if (!is_term || vt_check_enabled(print_term_msgs)) {
    vt_debug_print(
      active, node,
      "deliverActiveMsg: msg={}, handler={:x}, tag={}, is_auto={}, "
      "is_functor={}, is_obj_group={}, has_handler={}, insert={}\n",
      print_ptr(msg), handler, tag, is_auto, is_functor, is_obj,
      has_handler, insert
    );
  }

  if (has_handler) {
    // the epoch_stack_ size after the epoch on the active message, if included
    // in the envelope, is pushed.
    EpochStackSizeType ep_stack_size = 0;

    auto const env_epoch    = not (epoch == term::any_epoch_sentinel);
    auto const has_epoch    = env_epoch and epoch != no_epoch;
    auto const cur_epoch    = env_epoch ? epoch : no_epoch;

    // set the current handler so the user can request it in the context of an
    // active fun
    current_handler_context_  = handler;
    current_node_context_     = from_node;
    current_epoch_context_    = cur_epoch;

    #if vt_check_enabled(priorities)
      current_priority_context_       = envelopeGetPriority(msg->env);
      current_priority_level_context_ = envelopeGetPriorityLevel(msg->env);
    #endif

    #if vt_check_enabled(trace_enabled)
      current_trace_context_  = envelopeGetTraceEvent(msg->env);
    #endif

    if (has_epoch) {
      ep_stack_size = epochPreludeHandler(cur_epoch);
    }

    if (is_term) {
      tdRecvCount.increment(1);
    }
    amHandlerCount.increment(1);

    runnable::Runnable<MsgType>::run(handler,active_fun,msg,from_node,tag);

    // unset current handler
    current_handler_context_  = uninitialized_handler;
    current_node_context_     = uninitialized_destination;
    current_epoch_context_    = no_epoch;

    #if vt_check_enabled(trace_enabled)
      current_trace_context_  = trace::no_trace_event;
    #endif

    #if vt_check_enabled(priorities)
      current_priority_context_       = no_priority;
      current_priority_level_context_ = no_priority_level;
    #endif

    if (cont != nullptr) {
      cont();
    }

    if (has_epoch) {
      epochEpilogHandler(cur_epoch,ep_stack_size);
    }

    if (not is_term) {
      theTerm()->consume(epoch,1,from_node);
      theTerm()->hangDetectRecv();
    }
  } else {
    if (insert) {
      auto iter = pending_handler_msgs_.find(handler);
      if (iter == pending_handler_msgs_.end()) {
        pending_handler_msgs_.emplace(
          std::piecewise_construct,
          std::forward_as_tuple(handler),
          std::forward_as_tuple(
            MsgContType{BufferedMsgType{base,from_node,cont}}
          )
        );
      } else {
        iter->second.push_back(BufferedMsgType{base,from_node,cont});
      }
    }
  }

  return has_handler;
}

bool ActiveMessenger::tryProcessIncomingActiveMsg() {
  CountType num_probe_bytes;
  MPI_Status stat;
  int flag;

  {
    VT_ALLOW_MPI_CALLS;

    MPI_Iprobe(
      MPI_ANY_SOURCE, static_cast<MPI_TagType>(MPITag::ActiveMsgTag),
      theContext()->getComm(), &flag, &stat
    );
  }

  if (flag == 1) {
    MPI_Get_count(&stat, MPI_BYTE, &num_probe_bytes);

    #if vt_check_enabled(memory_pool)
      char* buf = static_cast<char*>(thePool()->alloc(num_probe_bytes));
    #else
      char* buf = static_cast<char*>(std::malloc(num_probe_bytes));
    #endif

    NodeType const sender = stat.MPI_SOURCE;

    MPI_Request req;

    {
      #if vt_check_enabled(trace_enabled)
        double tr_begin = 0;
        if (theConfig()->vt_trace_mpi) {
          tr_begin = vt::timing::Timing::getCurrentTime();
        }
      #endif

      VT_ALLOW_MPI_CALLS;
      MPI_Irecv(
        buf, num_probe_bytes, MPI_BYTE, sender, stat.MPI_TAG,
        theContext()->getComm(), &req
      );

      amPostedCounterGauge.incrementUpdate(num_probe_bytes, 1);

      #if vt_check_enabled(trace_enabled)
        if (theConfig()->vt_trace_mpi) {
          auto tr_end = vt::timing::Timing::getCurrentTime();
          auto tr_note = fmt::format(
            "Irecv(AM): from={}, bytes={}",
            stat.MPI_SOURCE, num_probe_bytes
          );
          trace::addUserBracketedNote(tr_begin, tr_end, tr_note, trace_irecv);
        }
      #endif
    }

    InProgressIRecv recv_holder{buf, num_probe_bytes, sender, req};

    int num_mpi_tests = 0;
    auto done = recv_holder.test(num_mpi_tests);
    amPollCount.increment(num_mpi_tests);

    if (done) {
      finishPendingActiveMsgAsyncRecv(&recv_holder);
    } else {
      in_progress_active_msg_irecv.emplace(std::move(recv_holder));
    }

    return true;
  } else {
    return false;
  }
}

void ActiveMessenger::finishPendingActiveMsgAsyncRecv(InProgressIRecv* irecv) {
  char* buf = irecv->buf;
  auto num_probe_bytes = irecv->probe_bytes;
  auto sender = irecv->sender;

  amRecvCounterGauge.incrementUpdate(num_probe_bytes, 1);

# if vt_check_enabled(trace_enabled)
  if (theConfig()->vt_trace_mpi) {
    auto tr_note = fmt::format("AM Irecv completed: from={}", irecv->sender);
    trace::addUserNote(tr_note);
  }
# endif

  MessageType* msg = reinterpret_cast<MessageType*>(buf);
  envelopeInitRecv(msg->env);
  MsgPtr<MessageType> base = MsgPtr<MessageType>{msg};

  auto const is_term = envelopeIsTerm(msg->env);
  auto const is_put = envelopeIsPut(msg->env);
  bool put_finished = false;

  if (!is_term || vt_check_enabled(print_term_msgs)) {
    vt_debug_print(
      active, node,
      "finishPendingActiveMsgAsyncRecv: msg_size={}, sender={}, is_put={}, "
      "is_bcast={}, handler={}\n",
      num_probe_bytes, sender, print_bool(is_put),
      print_bool(envelopeIsBcast(msg->env)), envelopeGetHandler(msg->env)
    );
  }

  CountType msg_bytes = num_probe_bytes;

  if (is_put) {
    auto const put_tag = envelopeGetPutTag(msg->env);
    if (put_tag == PutPackedTag) {
      auto const put_size = envelopeGetPutSize(msg->env);
      auto const msg_size = num_probe_bytes - put_size;
      char* put_ptr = buf + msg_size;
      msg_bytes = msg_size;

      if (!is_term || vt_check_enabled(print_term_msgs)) {
        vt_debug_print(
          active, node,
          "finishPendingActiveMsgAsyncRecv: packed put: ptr={}, msg_size={}, "
          "put_size={}\n",
          put_ptr, msg_size, put_size
        );
      }

      envelopeSetPutPtrOnly(msg->env, put_ptr);
      put_finished = true;
    } else {
      /*bool const put_delivered = */recvDataMsg(
        1, put_tag, sender,
        [=](PtrLenPairType ptr, ActionType deleter){
          envelopeSetPutPtr(base->env, std::get<0>(ptr), std::get<1>(ptr));
          scheduleActiveMsg(base, sender, num_probe_bytes, true, deleter);
        }
     );
    }
  }

  if (!is_put || put_finished) {
    scheduleActiveMsg(base, sender, msg_bytes, true);
  }
}

bool ActiveMessenger::testPendingActiveMsgAsyncRecv() {
  int num_mpi_tests = 0;
  bool const ret = in_progress_active_msg_irecv.testAll(
    [](InProgressIRecv* e){
      theMsg()->finishPendingActiveMsgAsyncRecv(e);
    },
    num_mpi_tests
  );
  amPollCount.increment(num_mpi_tests);
  return ret;
}

bool ActiveMessenger::testPendingDataMsgAsyncRecv() {
  int num_mpi_tests = 0;
  bool const ret = in_progress_data_irecv.testAll(
    [](InProgressDataIRecv* e){
      theMsg()->finishPendingDataMsgAsyncRecv(e);
    },
    num_mpi_tests
  );
  dmPollCount.increment(num_mpi_tests);
  return ret;
}

bool ActiveMessenger::testPendingAsyncOps() {
  int num_tests = 0;
  return in_progress_ops.testAll(
    [](AsyncOpWrapper* op) {
      op->done();
    },
    num_tests
  );
}

int ActiveMessenger::progress() {
  bool const started_irecv_active_msg = tryProcessIncomingActiveMsg();
  bool const started_irecv_data_msg = tryProcessDataMsgRecv();
  processMaybeReadyHanTag();
  bool const received_active_msg = testPendingActiveMsgAsyncRecv();
  bool const received_data_msg = testPendingDataMsgAsyncRecv();
  bool const general_async = testPendingAsyncOps();

  return started_irecv_active_msg or started_irecv_data_msg or
         received_active_msg or received_data_msg or general_async;
}

void ActiveMessenger::processMaybeReadyHanTag() {
  decltype(maybe_ready_tag_han_) maybe_ready = maybe_ready_tag_han_;
  // Clear first so clearing doesn't happen after new entries may be added by an
  // active message arriving
  maybe_ready_tag_han_.clear();
  for (auto&& x : maybe_ready) {
    deliverPendingMsgsHandler(std::get<0>(x), std::get<1>(x));
  }
}

HandlerType ActiveMessenger::registerNewHandler(
  ActiveClosureFnType fn, TagType const& tag
) {
  return theRegistry()->registerNewHandler(fn, tag);
}

HandlerType ActiveMessenger::collectiveRegisterHandler(
  ActiveClosureFnType fn, TagType const& tag
) {
  return theRegistry()->registerActiveHandler(fn, tag);
}

void ActiveMessenger::swapHandlerFn(
  HandlerType const han, ActiveClosureFnType fn, TagType const& tag
) {
  vt_debug_print(
    active, node,
    "swapHandlerFn: han={}, tag={}\n", han, tag
  );

  theRegistry()->swapHandler(han, fn, tag);

  if (fn != nullptr) {
    maybe_ready_tag_han_.push_back(ReadyHanTagType{han,tag});
  }
}

void ActiveMessenger::deliverPendingMsgsHandler(
  HandlerType const han, TagType const& tag
) {
  vt_debug_print(
    active, node,
    "deliverPendingMsgsHandler: han={}, tag={}\n", han, tag
  );
  auto iter = pending_handler_msgs_.find(han);
  if (iter != pending_handler_msgs_.end()) {
    if (iter->second.size() > 0) {
      for (auto cur = iter->second.begin(); cur != iter->second.end(); ) {
        vt_debug_print(
          active, node,
          "deliverPendingMsgsHandler: msg={}, from={}\n",
          print_ptr(cur->buffered_msg.get()), cur->from_node
        );
        if (
          deliverActiveMsg(cur->buffered_msg, cur->from_node, false, cur->cont)
        ) {
          cur = iter->second.erase(cur);
        } else {
          ++cur;
        }
      }
    } else {
      pending_handler_msgs_.erase(iter);
    }
  }
}

void ActiveMessenger::registerHandlerFn(
  HandlerType const han, ActiveClosureFnType fn, TagType const& tag
) {
  vt_debug_print(
    active, node,
    "registerHandlerFn: han={}, tag={}\n", han, tag
  );

  swapHandlerFn(han, fn, tag);

  if (fn != nullptr) {
    maybe_ready_tag_han_.push_back(ReadyHanTagType{han,tag});
  }
}

void ActiveMessenger::unregisterHandlerFn(
  HandlerType const han, TagType const& tag
) {
  vt_debug_print(
    active, node,
    "unregisterHandlerFn: han={}, tag={}\n", han, tag
  );

  return theRegistry()->unregisterHandlerFn(han, tag);
}

HandlerType ActiveMessenger::getCurrentHandler() const {
  return current_handler_context_;
}

EpochType ActiveMessenger::getCurrentEpoch() const {
  return current_epoch_context_;
}

PriorityType ActiveMessenger::getCurrentPriority() const {
  return current_priority_context_;
}

PriorityLevelType ActiveMessenger::getCurrentPriorityLevel() const {
  return current_priority_level_context_;
}

}} // end namespace vt::messaging
