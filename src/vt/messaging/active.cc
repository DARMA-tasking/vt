/*
//@HEADER
// ************************************************************************
//
//                          active.cc
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/


#include "vt/config.h"
#include "vt/messaging/active.h"
#include "vt/messaging/envelope.h"
#include "vt/messaging/message/smart_ptr.h"
#include "vt/termination/term_headers.h"
#include "vt/group/group_manager_active_attorney.h"
#include "vt/runnable/general.h"

namespace vt { namespace messaging {

ActiveMessenger::ActiveMessenger()
  : this_node_(theContext()->getNode())
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
  MessageType const msg, MsgSizeType const& size, void* ptr,
  MsgSizeType const& ptr_bytes
) {
  debug_print(
    active, node,
    "packMsg: msg_size={}, put_size={}, ptr={}\n",
    size, ptr_bytes, print_ptr(ptr)
  );

  char* const msg_buffer = reinterpret_cast<char* const>(msg) + size;
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
  auto& holder = theEvent()->getEventHolder(new_event);
  EventRecordType* parent = holder.get_event();

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
     *
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
      auto msg_size_ptr = static_cast<intptr_t>(msg_size);
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
      auto const& put_event_send = std::get<0>(ret);
      if (ret_tag != env_tag) {
        envelopeSetPutTag(msg->env, ret_tag);
      }
    }
  } else if (is_put && is_put_packed) {
    // Adjust size of the send for packed data
    auto const& put_size = envelopeGetPutSize(msg->env);
    new_msg_size += put_size;
  }

  auto const& send_event = sendMsgBytes(
    dest, base, new_msg_size, send_tag
  );

  return new_event;
}

EventType ActiveMessenger::sendMsgBytes(
  NodeType const& dest, MsgSharedPtr<BaseMsgType> const& base,
  MsgSizeType const& msg_size, TagType const& send_tag
) {
  auto const& msg = base.get();

  auto const& epoch = envelopeIsEpochType(msg->env) ?
    envelopeGetEpoch(msg->env) : term::any_epoch_sentinel;
  auto const& is_shared = isSharedMessage(msg);
  auto const& is_term = envelopeIsTerm(msg->env);
  auto const& is_bcast = envelopeIsBcast(msg->env);

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
    "Destination {} should != this node", dest
  );
  vtAbortIf(
    dest >= theContext()->getNumNodes() || dest < 0, "Invalid destination: {}", dest
  );

  MPI_Isend(
    msg, msg_size, MPI_BYTE, dest, send_tag, theContext()->getComm(),
    mpi_event->getRequest()
  );

  if (not is_term) {
    theTerm()->produce(epoch);
    theTerm()->send(dest,epoch);
  }

  return event_id;
}

#if backend_check_enabled(trace_enabled)
trace::TraceEventIDType ActiveMessenger::getCurrentTraceEvent() const {
  return current_trace_context_;
}
#endif

EventType ActiveMessenger::sendMsgSized(
  MsgSharedPtr<BaseMsgType> const& base, MsgSizeType const& msg_size
) {
  auto const& send_tag = static_cast<MPI_TagType>(MPITag::ActiveMsgTag);

  auto msg = base.get();

  auto const& dest = envelopeGetDest(msg->env);
  auto const& is_bcast = envelopeIsBcast(msg->env);
  auto const& is_term = envelopeIsTerm(msg->env);
  auto const& is_epoch = envelopeIsEpochType(msg->env);
  auto const& is_shared = isSharedMessage(msg);

  backend_enable_if(
    trace_enabled, {
      auto const& handler = envelopeGetHandler(msg->env);
      bool const& is_auto = HandlerManagerType::isHandlerAuto(handler);
      if (is_auto) {
        trace::TraceEntryIDType ep = auto_registry::theTraceID(
          handler, auto_registry::RegistryTypeEnum::RegGeneral
        );
        if (not is_bcast) {
          trace::TraceEventIDType event = theTrace()->messageCreation(ep, msg_size);
          envelopeSetTraceEvent(msg->env, event);
        } else if (is_bcast and dest == this_node_) {
          trace::TraceEventIDType event = theTrace()->messageCreationBcast(
            ep, msg_size
          );
          envelopeSetTraceEvent(msg->env, event);
        }
      }
    }
  );

  if (!is_term || backend_check_enabled(print_term_msgs)) {
    debug_print(
      active, node,
      "sendMsgSized: dest={}, handler={}, is_bcast={}, is_put={}\n",
      dest, envelopeGetHandler(msg->env), print_bool(is_bcast),
      print_bool(envelopeIsPut(msg->env))
    );
  }

  if (is_epoch) {
    // Propagate current epoch on the top of the epoch stack
    auto epoch = envelopeGetEpoch(msg->env);

    // Only propagate only if the epoch is not set already
    if (epoch == no_epoch) {
      auto const cur_epoch = getGlobalEpoch();
      if (cur_epoch != term::any_epoch_sentinel && cur_epoch != no_epoch) {
        setEpochMessage(msg, cur_epoch);
      }
    }
  }

  bool deliver = false;
  EventType const ret_event = group::GroupActiveAttorney::groupHandler(
    base, uninitialized_destination, msg_size, true, &deliver
  );

  EventType ret = no_event;

  if (deliver) {
    auto const& is_put = envelopeIsPut(msg->env);

    EventRecordType* parent = nullptr;
    EventType event = no_event;

    auto const send_put_event = sendMsgBytesWithPut(
      dest, base, msg_size, send_tag
    );

    ret = event;
  } else {
    ret = ret_event;
  }

  return ret;
}

ActiveMessenger::SendDataRetType ActiveMessenger::sendData(
  RDMA_GetType const& ptr, NodeType const& dest, TagType const& tag
) {
  auto const& data_ptr = std::get<0>(ptr);
  auto const& num_bytes = std::get<1>(ptr);
  auto const send_tag = tag == no_tag ? cur_direct_buffer_tag_++ : tag;

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
    "Destination {} should != this node", dest
  );
  vtAbortIf(
    dest >= theContext()->getNumNodes() || dest < 0,
    "Invalid destination: {}", dest
  );

  MPI_Isend(
    data_ptr, num_bytes, MPI_BYTE, dest, send_tag, theContext()->getComm(),
    mpi_event->getRequest()
  );

  theTerm()->produce(term::any_epoch_sentinel);

  return SendDataRetType{event_id,send_tag};
}

bool ActiveMessenger::recvDataMsg(
  TagType const& tag, NodeType const& node, RDMA_ContinuationDeleteType next
) {
  return recvDataMsg(tag, node, true, next);
}

bool ActiveMessenger::processDataMsgRecv() {
  bool erase = false;
  auto iter = pending_recvs_.begin();

  for (; iter != pending_recvs_.end(); ++iter) {
    auto const& done = recvDataMsgBuffer(
      iter->second.user_buf, iter->first, iter->second.recv_node,
      false, iter->second.dealloc_user_buf, iter->second.cont
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
  void* const user_buf, TagType const& tag, NodeType const& node,
  bool const& enqueue, ActionType dealloc_user_buf,
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

      MPI_Recv(
        buf, num_probe_bytes, MPI_BYTE, stat.MPI_SOURCE, stat.MPI_TAG,
        theContext()->getComm(), MPI_STATUS_IGNORE
      );

      auto dealloc_buf = [=]{
        debug_print(
          active, node,
          "recvDataMsgBuffer: continuation user_buf={}, buf={}, tag={}\n",
          user_buf, buf, tag
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

      if (next != nullptr) {
        next(RDMA_GetType{buf,num_probe_bytes}, [=]{
          dealloc_buf();
        });
      } else {
        dealloc_buf();
      }

      theTerm()->consume(term::any_epoch_sentinel);

      return true;
    } else {
      return false;
    }
  } else {
    debug_print(
      active, node,
      "recvDataMsgBuffer: node={}, tag={}, enqueue={}\n",
      node, tag, print_bool(enqueue)
    );

    pending_recvs_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(tag),
      std::forward_as_tuple(PendingRecvType{user_buf,next,dealloc_user_buf,node})
    );
    return false;
  }
}

bool ActiveMessenger::recvDataMsg(
  TagType const& tag, NodeType const& recv_node, bool const& enqueue,
  RDMA_ContinuationDeleteType next
) {
  return recvDataMsgBuffer(nullptr, tag, recv_node, enqueue, nullptr, next);
}

NodeType ActiveMessenger::getFromNodeCurrentHandler() const {
  return current_node_context_;
}

bool ActiveMessenger::handleActiveMsg(
  MsgSharedPtr<BaseMsgType> const& base, NodeType const& from,
  MsgSizeType const& size, bool insert
) {
  using ::vt::group::GroupActiveAttorney;

  auto msg = base.to<ShortMessage>().get();

  // Call group handler
  bool deliver = false;
  GroupActiveAttorney::groupHandler(base, from, size, false, &deliver);

  auto const& is_term = envelopeIsTerm(msg->env);

  if (!is_term || backend_check_enabled(print_term_msgs)) {
    debug_print(
      active, node,
      "handleActiveMsg: msg={}, ref={}, deliver={}\n",
      print_ptr(msg), envelopeGetRef(msg->env), print_bool(deliver)
    );
  }

  return deliver ? deliverActiveMsg(base,from,insert) : false;
}

bool ActiveMessenger::deliverActiveMsg(
  MsgSharedPtr<BaseMsgType> const& base, NodeType const& in_from_node,
  bool insert
) {
  auto msg = base.to<ShortMessage>().get();

  auto const& is_term = envelopeIsTerm(msg->env);
  auto const& is_bcast = envelopeIsBcast(msg->env);
  auto const& dest = envelopeGetDest(msg->env);
  auto const& handler = envelopeGetHandler(msg->env);
  auto const& epoch = envelopeIsEpochType(msg->env) ?
    envelopeGetEpoch(msg->env) : term::any_epoch_sentinel;
  auto const& is_tag = envelopeIsTagType(msg->env);
  auto const& tag = is_tag ? envelopeGetTag(msg->env) : no_tag;
  auto const& from_node = is_bcast ? dest : in_from_node;

  ActiveFnPtrType active_fun = nullptr;

  bool has_ex_handler = false;
  bool const& is_auto = HandlerManagerType::isHandlerAuto(handler);
  bool const& is_functor = HandlerManagerType::isHandlerFunctor(handler);
  bool const& is_obj = HandlerManagerType::isHandlerObjGroup(handler);

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

  bool const& has_handler = active_fun != no_action or has_ex_handler or is_obj;

  if (!is_term || backend_check_enabled(print_term_msgs)) {
    debug_print(
      active, node,
      "deliverActiveMsg: msg={}, handler={}, tag={}, is_auto={}, "
      "is_functor={}, has_handler={}, insert={}\n",
      print_ptr(msg), handler, tag, print_bool(is_auto),
      print_bool(is_functor), print_bool(has_handler),
      print_bool(insert)
    );
  }

  if (has_handler) {
    // the epoch_stack_ size after the epoch on the active message, if included
    // in the envelope, is pushed.
    EpochStackSizeType ep_stack_size = 0;

    auto const& env_epoch    = not (epoch == term::any_epoch_sentinel);
    auto const& has_epoch    = env_epoch and epoch != no_epoch;
    auto const& cur_epoch    = env_epoch ? epoch : no_epoch;

    // set the current handler so the user can request it in the context of an
    // active fun
    current_handler_context_  = handler;
    current_node_context_     = from_node;
    current_epoch_context_    = cur_epoch;

    backend_enable_if(
      trace_enabled,
      current_trace_context_  = envelopeGetTraceEvent(msg->env);
    );

    if (has_epoch) {
      ep_stack_size = epochPreludeHandler(cur_epoch);
    }

    if (is_obj) {
      // run the object-group handler
      runnable::Runnable<ShortMessage>::run(handler,active_fun,msg,from_node,tag);
    } else {
      // run the normal active function handler
      runnable::Runnable<ShortMessage>::run(handler,active_fun,msg,from_node,tag);
    }

    auto trigger = theRegistry()->getTrigger(handler);
    if (trigger) {
      trigger(msg);
    }

    // unset current handler
    current_handler_context_  = uninitialized_handler;
    current_node_context_     = uninitialized_destination;
    current_epoch_context_    = no_epoch;

    backend_enable_if(
      trace_enabled,
      current_trace_context_  = trace::no_trace_event;
    );

    if (has_epoch) {
      epochEpilogHandler(cur_epoch,ep_stack_size);
    }

  } else {
    if (insert) {
      auto iter = pending_handler_msgs_.find(handler);
      if (iter == pending_handler_msgs_.end()) {
        pending_handler_msgs_.emplace(
          std::piecewise_construct,
          std::forward_as_tuple(handler),
          std::forward_as_tuple(MsgContType{BufferedMsgType{base,from_node}})
        );
      } else {
        iter->second.push_back(BufferedMsgType{base,from_node});
      }
      if (!is_term || backend_check_enabled(print_term_msgs)) {
        debug_print(
          active, node,
          "deliverActiveMsg: inserting han={}, msg={}, ref={}, list size={}\n",
          handler, print_ptr(msg), envelopeGetRef(msg->env),
          pending_handler_msgs_.find(handler)->second.size()
        );
      }
    }
  }

  if (!is_term) {
    theTerm()->recv(from_node,epoch);
  }

  if (has_handler) {
    if (!is_term || backend_check_enabled(print_term_msgs)) {
      debug_print(
        active, node,
        "deliverActiveMsg: deref msg={}, ref={}, is_bcast={}, dest={}\n",
        print_ptr(msg), envelopeGetRef(msg->env), print_bool(is_bcast), dest
      );
    }

    if (!is_term) {
      theTerm()->consume(epoch);
    }
  }

  return has_handler;
}

bool ActiveMessenger::tryProcessIncomingMessage() {
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

    NodeType const& sender = stat.MPI_SOURCE;

    MPI_Recv(
      buf, num_probe_bytes, MPI_BYTE, sender, stat.MPI_TAG,
      theContext()->getComm(), MPI_STATUS_IGNORE
    );

    auto msg = reinterpret_cast<MessageType>(buf);
    messageConvertToShared(msg);
    auto base = promoteMsgOwner(msg);

    auto const& is_term = envelopeIsTerm(msg->env);
    auto const& is_put = envelopeIsPut(msg->env);
    bool put_finished = false;

    if (!is_term || backend_check_enabled(print_term_msgs)) {
      debug_print(
        active, node,
        "tryProcessIncoming: msg_size={}, sender={}, is_put={}, is_bcast={}, "
        "handler={}\n",
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
            "tryProcessIncoming: packed put: ptr={}, msg_size={}, put_size={}\n",
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
            handleActiveMsg(base, sender, num_probe_bytes, true);
          }
        );
      }
    }

    if (!is_put || put_finished) {
      handleActiveMsg(base, sender, msg_bytes, true);
    }

    return true;
  } else {
    return false;
  }
}

bool ActiveMessenger::scheduler() {
  bool const processed = tryProcessIncomingMessage();
  bool const processed_data_msg = processDataMsgRecv();
  processMaybeReadyHanTag();

  return processed or processed_data_msg;
}

bool ActiveMessenger::isLocalTerm() {
  bool const no_pending_msgs = pending_handler_msgs_.size() == 0;
  bool const no_pending_recvs = pending_recvs_.size() == 0;
  return no_pending_msgs and no_pending_recvs;
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
        if (deliverActiveMsg(cur->buffered_msg, cur->from_node, false)) {
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

}} // end namespace vt::messaging
