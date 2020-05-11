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
#include "vt/messaging/active.h"
#include "vt/messaging/envelope.h"
#include "vt/messaging/message/smart_ptr.h"
#include "vt/termination/term_headers.h"
#include "vt/group/group_manager_active_attorney.h"
#include "vt/runnable/general.h"
#include "vt/timing/timing.h"
#include "vt/scheduler/priority.h"
#include "vt/utils/mpi_limits/mpi_max_tag.h"

namespace vt { namespace messaging {

ActiveMessenger::ActiveMessenger()
  :
# if backend_check_enabled(trace_enabled)
  trace_irecv(trace::registerEventCollective("MPI_Irecv")),
  trace_isend(trace::registerEventCollective("MPI_Isend")),
  trace_irecv_polling_am(trace::registerEventCollective("IRecv: Active Msg poll")),
  trace_irecv_polling_dm(trace::registerEventCollective("IRecv: Data Msg poll")),
  in_progress_active_msg_irecv(trace_irecv_polling_am),
  in_progress_data_irecv(trace_irecv_polling_dm),
# endif
  this_node_(theContext()->getNode())
{
  /*
   * Push the default epoch into the stack so it is always at the bottom of the
   * stack during execution until the AM's destructor is invoked
   */
  pushEpoch(term::any_epoch_sentinel);
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
  debug_print(
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

  if (!is_term || backend_check_enabled(print_term_msgs)) {
    debug_print(
      active, node,
      "sendMsgBytesWithPut: size={}, dest={}, is_put={}, is_put_packed={}\n",
      msg_size, dest, print_bool(is_put), print_bool(is_put_packed)
    );
  }

  EventType new_event = theEvent()->createParentEvent(this_node_);

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
    if (!is_term || backend_check_enabled(print_term_msgs)) {
      debug_print(
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
        RDMA_GetType{put_ptr,put_size}, dest, env_tag
      );
      auto const& ret_tag = std::get<1>(ret);
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

  return new_event;
}

EventType ActiveMessenger::sendMsgBytes(
  NodeType const& dest, MsgSharedPtr<BaseMsgType> const& base,
  MsgSizeType const& msg_size, TagType const& send_tag
) {
  auto const& msg = base.get();

  auto const epoch = envelopeIsEpochType(msg->env) ?
    envelopeGetEpoch(msg->env) : term::any_epoch_sentinel;
  auto const is_shared = isSharedMessage(msg);
  auto const is_term = envelopeIsTerm(msg->env);
  auto const is_bcast = envelopeIsBcast(msg->env);

  auto const event_id = theEvent()->createMPIEvent(this_node_);
  auto& holder = theEvent()->getEventHolder(event_id);
  auto mpi_event = holder.get_event();

  if (!is_term || backend_check_enabled(print_term_msgs)) {
    debug_print(
      active, node,
      "sendMsgBytes: size={}, dest={}\n", msg_size, dest
    );
  }

  if (is_shared) {
    mpi_event->setManagedMessage(base.to<ShortMessage>());
  }

  vtWarnIf(
    !(dest != theContext()->getNode() || is_bcast),
    "Destination {} should != this node"
  );
  vtAbortIf(
    dest >= theContext()->getNumNodes() || dest < 0, "Invalid destination: {}"
  );

  {
    #if backend_check_enabled(trace_enabled)
      double tr_begin = 0;
      if (ArgType::vt_trace_mpi) {
        tr_begin = vt::timing::Timing::getCurrentTime();
      }
    #endif

    MPI_Isend(
      msg, msg_size, MPI_BYTE, dest, send_tag, theContext()->getComm(),
      mpi_event->getRequest()
    );

    #if backend_check_enabled(trace_enabled)
      if (ArgType::vt_trace_mpi) {
        auto tr_end = vt::timing::Timing::getCurrentTime();
        auto tr_note = fmt::format("Isend(AM): dest={}, bytes={}", dest, msg_size);
        trace::addUserBracketedNote(tr_begin, tr_end, tr_note, trace_isend);
      }
    #endif
  }

  if (not is_term) {
    theTerm()->produce(epoch,1,dest);
    theTerm()->hangDetectSend();
  }

  for (auto&& l : send_listen_) {
    l->send(dest, msg_size, is_bcast);
  }

  return event_id;
}

#if backend_check_enabled(trace_enabled)
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
  auto const is_epoch = envelopeIsEpochType(msg->env);

  #if backend_check_enabled(trace_enabled)
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
  #endif

  if (!is_term || backend_check_enabled(print_term_msgs)) {
    debug_print(
      active, node,
      "doMessageSend: dest={}, handler={:x}, is_bcast={}, is_put={}\n",
      dest, envelopeGetHandler(msg->env), print_bool(is_bcast),
      print_bool(envelopeIsPut(msg->env))
    );
  }

  if (is_epoch) {
    setupEpochMsg(msg);
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

ActiveMessenger::SendDataRetType ActiveMessenger::sendData(
  RDMA_GetType const& ptr, NodeType const& dest, TagType const& tag
) {
  auto const& data_ptr = std::get<0>(ptr);
  auto const& num_bytes = std::get<1>(ptr);

  int send_tag = 0;
  if (tag == no_tag) {
    send_tag = tag;
  } else {
    auto const max_tag = util::MPI_Attr::getMaxTag();
    send_tag = cur_direct_buffer_tag_++;

    // If max, wrap around back to the starting tag
    if (cur_direct_buffer_tag_ >= max_tag) {
      cur_direct_buffer_tag_ = starting_direct_buffer_tag;
    }
  }

  auto const event_id = theEvent()->createMPIEvent(this_node_);
  auto& holder = theEvent()->getEventHolder(event_id);

  auto mpi_event = holder.get_event();

  debug_print(
    active, node,
    "sendData: ptr={}, num_bytes={} dest={}, tag={}, send_tag={}\n",
    data_ptr, num_bytes, dest, tag, send_tag
  );

  vtWarnIf(
    dest == theContext()->getNode(),
    "Destination {} should != this node"
  );
  vtAbortIf(
    dest >= theContext()->getNumNodes() || dest < 0,
    "Invalid destination: {}"
  );

  {
    #if backend_check_enabled(trace_enabled)
      double tr_begin = 0;
      if (ArgType::vt_trace_mpi) {
        tr_begin = vt::timing::Timing::getCurrentTime();
      }
    #endif

    MPI_Isend(
      data_ptr, num_bytes, MPI_BYTE, dest, send_tag, theContext()->getComm(),
      mpi_event->getRequest()
    );

    #if backend_check_enabled(trace_enabled)
      if (ArgType::vt_trace_mpi) {
        auto tr_end = vt::timing::Timing::getCurrentTime();
        auto tr_note = fmt::format("Isend(Data): dest={}, bytes={}", dest, num_bytes);
        trace::addUserBracketedNote(tr_begin, tr_end, tr_note, trace_isend);
      }
    #endif
  }

  // Assume that any raw data send/recv is paired with a message with an epoch
  // if required to inhibit early termination of that epoch
  theTerm()->produce(term::any_epoch_sentinel,1,dest);
  theTerm()->hangDetectSend();

  for (auto&& l : send_listen_) {
    l->send(dest, num_bytes, false);
  }

  return SendDataRetType{event_id,send_tag};
}

bool ActiveMessenger::recvDataMsgPriority(
  PriorityType priority, TagType const& tag, NodeType const& node,
  RDMA_ContinuationDeleteType next
) {
  return recvDataMsg(priority, tag, node, true, next);
}

bool ActiveMessenger::recvDataMsg(
  TagType const& tag, NodeType const& node, RDMA_ContinuationDeleteType next
) {
  return recvDataMsg(default_priority, tag, node, true, next);
}

bool ActiveMessenger::tryProcessDataMsgRecv() {
  bool erase = false;
  auto iter = pending_recvs_.begin();

  for (; iter != pending_recvs_.end(); ++iter) {
    auto const done = recvDataMsgBuffer(
      iter->second.user_buf, iter->second.priority, iter->first,
      iter->second.recv_node, false, iter->second.dealloc_user_buf,
      iter->second.cont
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
  void* const user_buf, TagType const& tag,
  NodeType const& node, bool const& enqueue, ActionType dealloc,
  RDMA_ContinuationDeleteType next
) {
  return recvDataMsgBuffer(user_buf, no_priority, tag, node, enqueue, dealloc, next);
}

bool ActiveMessenger::recvDataMsgBuffer(
  void* const user_buf, PriorityType priority, TagType const& tag,
  NodeType const& node, bool const& enqueue, ActionType dealloc_user_buf,
  RDMA_ContinuationDeleteType next
) {
  if (not enqueue) {
    CountType num_probe_bytes;
    MPI_Status stat;
    int flag;

    MPI_Iprobe(
      node == uninitialized_destination ? MPI_ANY_SOURCE : node,
      tag, theContext()->getComm(), &flag, &stat
    );

    if (flag == 1) {
      MPI_Get_count(&stat, MPI_BYTE, &num_probe_bytes);

      char* buf =
        user_buf == nullptr ?

    #if backend_check_enabled(memory_pool)
        static_cast<char*>(thePool()->alloc(num_probe_bytes)) :
    #else
        static_cast<char*>(std::malloc(num_probe_bytes))      :
    #endif

        static_cast<char*>(user_buf);

      NodeType const sender = stat.MPI_SOURCE;

      MPI_Request req;

      {
        #if backend_check_enabled(trace_enabled)
          double tr_begin = 0;
          if (ArgType::vt_trace_mpi) {
            tr_begin = vt::timing::Timing::getCurrentTime();
          }
        #endif

        MPI_Irecv(
          buf, num_probe_bytes, MPI_BYTE, stat.MPI_SOURCE, stat.MPI_TAG,
          theContext()->getComm(), &req
        );

        #if backend_check_enabled(trace_enabled)
          if (ArgType::vt_trace_mpi) {
            auto tr_end = vt::timing::Timing::getCurrentTime();
            auto tr_note = fmt::format(
              "Irecv(Data): from={}, bytes={}",
              stat.MPI_SOURCE, num_probe_bytes
            );
            trace::addUserBracketedNote(tr_begin, tr_end, tr_note, trace_irecv);
          }
        #endif
      }

      InProgressDataIRecv recv_holder{
        buf, num_probe_bytes, sender, req, user_buf, dealloc_user_buf, next,
        priority
      };

      int recv_flag = 0;
      MPI_Status recv_stat;
      MPI_Test(&recv_holder.req, &recv_flag, &recv_stat);
      if (recv_flag == 1) {
        finishPendingDataMsgAsyncRecv(&recv_holder);
      } else {
        in_progress_data_irecv.emplace(std::move(recv_holder));
      }

      return true;
    } else {
      return false;
    }
  } else {
    debug_print(
      active, node,
      "recvDataMsgBuffer: node={}, tag={}, enqueue={}, priority={:x}\n",
      node, tag, print_bool(enqueue), priority
    );

    pending_recvs_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(tag),
      std::forward_as_tuple(
        PendingRecvType{user_buf,next,dealloc_user_buf,node,priority}
      )
    );
    return false;
  }
}

void ActiveMessenger::finishPendingDataMsgAsyncRecv(InProgressDataIRecv* irecv) {
  auto buf = irecv->buf;
  auto num_probe_bytes = irecv->probe_bytes;
  auto sender = irecv->sender;
  auto user_buf = irecv->user_buf;
  auto dealloc_user_buf = irecv->dealloc_user_buf;
  auto next = irecv->next;

# if backend_check_enabled(trace_enabled)
  if (ArgType::vt_trace_mpi) {
    auto tr_note = fmt::format("DM Irecv completed: from={}", irecv->sender);
    trace::addUserNote(tr_note);
  }
# endif

  auto dealloc_buf = [=]{
    debug_print(
      active, node,
      "finishPendingDataMsgAsyncRecv: continuation user_buf={}, buf={}\n",
      user_buf, buf
    );

    if (user_buf == nullptr) {
      #if backend_check_enabled(memory_pool)
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
      next(RDMA_GetType{buf,num_probe_bytes}, dealloc_buf);
      theTerm()->consume(term::any_epoch_sentinel,1,sender);
      theTerm()->hangDetectRecv();
    };
    theSched()->enqueue(irecv->priority, run);
  }
}

bool ActiveMessenger::recvDataMsg(
  PriorityType priority, TagType const& tag, NodeType const& recv_node,
  bool const& enqueue, RDMA_ContinuationDeleteType next
) {
  return recvDataMsgBuffer(
    nullptr, priority, tag, recv_node, enqueue, nullptr, next
  );
}

NodeType ActiveMessenger::getFromNodeCurrentHandler() const {
  return current_node_context_;
}

void ActiveMessenger::scheduleActiveMsg(
  MsgSharedPtr<BaseMsgType> const& base, NodeType const& from,
  MsgSizeType const& size, bool insert, ActionType cont
) {
  debug_print_verbose(
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

  if (!is_term || backend_check_enabled(print_term_msgs)) {
    debug_print(
      active, node,
      "processActiveMsg: msg={}, ref={}, deliver={}\n",
      print_ptr(msg), envelopeGetRef(msg->env), print_bool(deliver)
    );
  }

  if (deliver) {
    return deliverActiveMsg(base,from,insert,cont);
  } else {
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

  if (!is_term || backend_check_enabled(print_term_msgs)) {
    debug_print(
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

  if (!is_term || backend_check_enabled(print_term_msgs)) {
    debug_print(
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

    #if backend_check_enabled(priorities)
      current_priority_context_       = envelopeGetPriority(msg->env);
      current_priority_level_context_ = envelopeGetPriorityLevel(msg->env);
    #endif

    #if backend_check_enabled(trace_enabled)
      current_trace_context_  = envelopeGetTraceEvent(msg->env);
    #endif

    if (has_epoch) {
      ep_stack_size = epochPreludeHandler(cur_epoch);
    }

    runnable::Runnable<MsgType>::run(handler,active_fun,msg,from_node,tag);

    // unset current handler
    current_handler_context_  = uninitialized_handler;
    current_node_context_     = uninitialized_destination;
    current_epoch_context_    = no_epoch;

    #if backend_check_enabled(trace_enabled)
      current_trace_context_  = trace::no_trace_event;
    #endif

    #if backend_check_enabled(priorities)
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

  MPI_Iprobe(
    MPI_ANY_SOURCE, static_cast<MPI_TagType>(MPITag::ActiveMsgTag),
    theContext()->getComm(), &flag, &stat
  );

  if (flag == 1) {
    MPI_Get_count(&stat, MPI_BYTE, &num_probe_bytes);

    #if backend_check_enabled(memory_pool)
      char* buf = static_cast<char*>(thePool()->alloc(num_probe_bytes));
    #else
      char* buf = static_cast<char*>(std::malloc(num_probe_bytes));
    #endif

    NodeType const sender = stat.MPI_SOURCE;

    MPI_Request req;

    {
      #if backend_check_enabled(trace_enabled)
        double tr_begin = 0;
        if (ArgType::vt_trace_mpi) {
          tr_begin = vt::timing::Timing::getCurrentTime();
        }
      #endif

      MPI_Irecv(
        buf, num_probe_bytes, MPI_BYTE, sender, stat.MPI_TAG,
        theContext()->getComm(), &req
      );

      #if backend_check_enabled(trace_enabled)
        if (ArgType::vt_trace_mpi) {
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

    int recv_flag = 0;
    MPI_Status recv_stat;
    MPI_Test(&recv_holder.req, &recv_flag, &recv_stat);
    if (recv_flag == 1) {
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
  auto buf = irecv->buf;
  auto num_probe_bytes = irecv->probe_bytes;
  auto sender = irecv->sender;

# if backend_check_enabled(trace_enabled)
  if (ArgType::vt_trace_mpi) {
    auto tr_note = fmt::format("AM Irecv completed: from={}", irecv->sender);
    trace::addUserNote(tr_note);
  }
# endif

  MessageType* msg = reinterpret_cast<MessageType*>(buf);
  messageConvertToShared(msg);
  auto base = promoteMsgOwner(msg);

  auto const is_term = envelopeIsTerm(msg->env);
  auto const is_put = envelopeIsPut(msg->env);
  bool put_finished = false;

  if (!is_term || backend_check_enabled(print_term_msgs)) {
    debug_print(
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

      if (!is_term || backend_check_enabled(print_term_msgs)) {
        debug_print(
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
        put_tag, sender,
        [=](RDMA_GetType ptr, ActionType deleter){
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
  return in_progress_active_msg_irecv.testAll(
    [](InProgressIRecv* e){
      theMsg()->finishPendingActiveMsgAsyncRecv(e);
    }
  );
}

bool ActiveMessenger::testPendingDataMsgAsyncRecv() {
  return in_progress_data_irecv.testAll(
    [](InProgressDataIRecv* e){
      theMsg()->finishPendingDataMsgAsyncRecv(e);
    }
  );
}

int ActiveMessenger::progress() {
  bool const started_irecv_active_msg = tryProcessIncomingActiveMsg();
  bool const started_irecv_data_msg = tryProcessDataMsgRecv();
  processMaybeReadyHanTag();
  bool const received_active_msg = testPendingActiveMsgAsyncRecv();
  bool const received_data_msg = testPendingDataMsgAsyncRecv();

  return started_irecv_active_msg or started_irecv_data_msg or
         received_active_msg or received_data_msg;
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
  HandlerType const& han, ActiveClosureFnType fn, TagType const& tag
) {
  debug_print(
    active, node,
    "swapHandlerFn: han={}, tag={}\n", han, tag
  );

  theRegistry()->swapHandler(han, fn, tag);

  if (fn != nullptr) {
    maybe_ready_tag_han_.push_back(ReadyHanTagType{han,tag});
  }
}

void ActiveMessenger::deliverPendingMsgsHandler(
  HandlerType const& han, TagType const& tag
) {
  debug_print(
    active, node,
    "deliverPendingMsgsHandler: han={}, tag={}\n", han, tag
  );
  auto iter = pending_handler_msgs_.find(han);
  if (iter != pending_handler_msgs_.end()) {
    if (iter->second.size() > 0) {
      for (auto cur = iter->second.begin(); cur != iter->second.end(); ) {
        debug_print(
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
  HandlerType const& han, ActiveClosureFnType fn, TagType const& tag
) {
  debug_print(
    active, node,
    "registerHandlerFn: han={}, tag={}\n", han, tag
  );

  swapHandlerFn(han, fn, tag);

  if (fn != nullptr) {
    maybe_ready_tag_han_.push_back(ReadyHanTagType{han,tag});
  }
}

void ActiveMessenger::unregisterHandlerFn(
  HandlerType const& han, TagType const& tag
) {
  debug_print(
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
