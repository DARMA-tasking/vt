
#include "config.h"
#include "messaging/active.h"
#include "messaging/envelope.h"
#include "termination/term_headers.h"
#include "group/group_manager_active_attorney.h"
#include "runnable/general.h"

namespace vt { namespace messaging {

ActiveMessenger::ActiveMessenger() : this_node_(theContext()->getNode()) { }

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
  NodeType const& dest, BaseMessage* const base, MsgSizeType const& msg_size,
  TagType const& send_tag, ActionType next_action
) {
  auto msg = reinterpret_cast<MessageType const>(base);
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
    auto const& rem_size = thePool()->remainingSize(base);
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
    assert(
      (!(put_size != 0) || put_ptr) && "Must have valid ptr if size > 0"
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
        RDMA_GetType{put_ptr,put_size}, dest, env_tag, nullptr
      );
      auto const& ret_tag = std::get<1>(ret);
      auto const& put_event_send = std::get<0>(ret);
      if (ret_tag != env_tag) {
        envelopeSetPutTag(msg->env, ret_tag);
      }
      if (next_action) {
        parent->addEventToList(put_event_send);
      }
    }
  } else if (is_put && is_put_packed) {
    // Adjust size of the send for packed data
    auto const& put_size = envelopeGetPutSize(msg->env);
    new_msg_size += put_size;
  }

  auto const& send_event = sendMsgBytes(
    dest, msg, new_msg_size, send_tag, next_action
  );

  if (next_action) {
    parent->addEventToList(send_event);
  }

  return new_event;
}

EventType ActiveMessenger::sendMsgBytes(
  NodeType const& dest, BaseMessage* const base, MsgSizeType const& msg_size,
  TagType const& send_tag, ActionType next_action
) {
  auto const& msg = reinterpret_cast<MessageType const>(base);

  auto const& epoch = envelopeIsEpochType(msg->env) ?
    envelopeGetEpoch(msg->env) : term::any_epoch_sentinel;
  auto const& is_shared = isSharedMessage(msg);
  auto const& is_term = envelopeIsTerm(msg->env);

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
    mpi_event->setManagedMessage(msg);
  }

  if (not is_term) {
    theTerm()->produce(epoch);
  }

  #if backend_check_enabled(runtime_checks)
  assert(
    dest != theContext()->getNode() && "Destination should not be this node"
  );
  #endif

  MPI_Isend(
    msg, msg_size, MPI_BYTE, dest, send_tag, theContext()->getComm(),
    mpi_event->getRequest()
  );

  if (is_shared) {
    messageDeref(msg);
  }

  return event_id;
}

#if backend_check_enabled(trace_enabled)
trace::TraceEventIDType ActiveMessenger::getCurrentTraceEvent() const {
  return current_trace_context_;
}
#endif

EventType ActiveMessenger::sendMsgSized(
  HandlerType const& han, BaseMessage* const base, MsgSizeType const& msg_size,
  ActionType next_action
) {
  auto const& send_tag = static_cast<MPI_TagType>(MPITag::ActiveMsgTag);

  auto msg = reinterpret_cast<MessageType const>(base);

  auto const& dest = envelopeGetDest(msg->env);
  auto const& is_bcast = envelopeIsBcast(msg->env);
  auto const& is_term = envelopeIsTerm(msg->env);
  auto const& is_epoch = envelopeIsEpochType(msg->env);

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
    // Propagate current epoch on message first, otherwise propagate the global
    // epoch set on the active messenger
    if (current_epoch_context_ != no_epoch) {
      setEpochMessage(msg, current_epoch_context_);
    } else if (global_epoch_ != no_epoch) {
      setEpochMessage(msg, global_epoch_);
    }
  }

  bool deliver = false;
  EventType const ret_event = group::GroupActiveAttorney::groupHandler(
    msg, uninitialized_destination, msg_size, true, next_action, &deliver
  );

  if (deliver) {
    auto const& is_put = envelopeIsPut(msg->env);

    EventRecordType* parent = nullptr;
    EventType event = no_event;

    if (next_action) {
      event = theEvent()->createParentEvent(this_node_);
      auto& holder = theEvent()->getEventHolder(event);
      parent = holder.get_event();
    }

    auto const send_put_event = sendMsgBytesWithPut(
      dest, base, msg_size, send_tag, next_action
    );

    if (next_action) {
      if (ret_event != no_event) {
        parent->addEventToList(ret_event);
      }
      if (send_put_event != no_event) {
        parent->addEventToList(send_put_event);
      }
      auto& holder = theEvent()->getEventHolder(event);
      holder.attachAction(next_action);
    }

    return event;
  } else {
    if (ret_event != no_event && next_action) {
      auto& holder = theEvent()->getEventHolder(ret_event);
      holder.attachAction(next_action);
    }

    return ret_event;
  }
}

ActiveMessenger::SendDataRetType ActiveMessenger::sendData(
  RDMA_GetType const& ptr, NodeType const& dest, TagType const& tag,
  ActionType next_action
) {
  auto const& data_ptr = std::get<0>(ptr);
  auto const& num_bytes = std::get<1>(ptr);
  auto const send_tag = tag == no_tag ? cur_direct_buffer_tag_++ : tag;

  auto const event_id = theEvent()->createMPIEvent(this_node_);
  auto& holder = theEvent()->getEventHolder(event_id);

  if (next_action != nullptr) {
    holder.attachAction(next_action);
  }

  auto mpi_event = holder.get_event();

  debug_print(
    active, node,
    "sendData: ptr={}, num_bytes={} dest={}, tag={}, send_tag={}\n",
    data_ptr, num_bytes, dest, tag, send_tag
  );

  #if backend_check_enabled(runtime_checks)
  assert(
    dest != theContext()->getNode() && "Destination should not be this node"
  );
  #endif

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
        static_cast<char*>(thePool()->alloc(num_probe_bytes)) :
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
          thePool()->dealloc(buf);
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
  MessageType msg, NodeType const& from, MsgSizeType const& size, bool insert
) {
  using ::vt::group::GroupActiveAttorney;

  // Call group handler
  bool deliver = false;
  GroupActiveAttorney::groupHandler(msg, from, size, false, nullptr, &deliver);

  auto const& is_term = envelopeIsTerm(msg->env);

  if (!is_term || backend_check_enabled(print_term_msgs)) {
    debug_print(
      active, node,
      "handleActiveMsg: msg={}, ref={}, deliver={}\n",
      print_ptr(msg), envelopeGetRef(msg->env), print_bool(deliver)
    );
  }

  if (deliver) {
    auto const& ret = deliverActiveMsg(msg, from, insert);
    return ret;
  } else {
    messageDeref(msg);
    return false;
  }
}

bool ActiveMessenger::deliverActiveMsg(
  MessageType msg, NodeType const& in_from_node, bool insert
) {
  auto const& is_term = envelopeIsTerm(msg->env);
  auto const& is_bcast = envelopeIsBcast(msg->env);
  auto const& dest = envelopeGetDest(msg->env);
  auto const& handler = envelopeGetHandler(msg->env);
  auto const& epoch = envelopeIsEpochType(msg->env) ?
    envelopeGetEpoch(msg->env) : term::any_epoch_sentinel;
  auto const& is_tag = envelopeIsTagType(msg->env);
  auto const& tag = is_tag ? envelopeGetTag(msg->env) : no_tag;
  auto const& callback =
    envelopeIsCallbackType(msg->env) ?
    CallbackMessage::getCallbackMessage(msg) : uninitialized_handler;
  auto const& from_node = is_bcast ? dest : in_from_node;

  ActiveFnPtrType active_fun = nullptr;

  bool has_ex_handler = false;
  bool const& is_auto = HandlerManagerType::isHandlerAuto(handler);
  bool const& is_functor = HandlerManagerType::isHandlerFunctor(handler);

  if (!is_term || backend_check_enabled(print_term_msgs)) {
    debug_print(
      active, node,
      "deliverActiveMsg: msg={}, ref={}, is_bcast={}\n",
      print_ptr(msg), envelopeGetRef(msg->env), print_bool(is_bcast)
    );
  }

  if (is_auto and is_functor) {
    active_fun = auto_registry::getAutoHandlerFunctor(handler);
  } else if (is_auto) {
    active_fun = auto_registry::getAutoHandler(handler);
  } else {
    auto ret = theRegistry()->getHandler(handler, tag);
    if (ret != nullptr) {
      has_ex_handler = true;
    }
  }

  bool const& has_action_handler = active_fun != no_action or has_ex_handler;

  if (!is_term || backend_check_enabled(print_term_msgs)) {
    debug_print(
      active, node,
      "deliverActiveMsg: msg={}, handler={}, tag={}, is_auto={}, "
      "is_functor={}, has_action_handler={}, insert={}\n",
      print_ptr(msg), handler, tag, print_bool(is_auto),
      print_bool(is_functor), print_bool(has_action_handler),
      print_bool(insert)
    );
  }

  if (has_action_handler) {
    // set the current handler so the user can request it in the context of an
    // active fun
    current_handler_context_  = handler;
    current_callback_context_ = callback;
    current_node_context_     = from_node;
    current_epoch_context_    = epoch;

    #if backend_check_enabled(trace_enabled)
      current_trace_context_  = from_node;
    #endif

    // run the active function
    runnable::Runnable<ShortMessage>::run(handler,active_fun,msg,from_node,tag);

    auto trigger = theRegistry()->getTrigger(handler);
    if (trigger) {
      trigger(msg);
    }

    // unset current handler
    current_handler_context_  = uninitialized_handler;
    current_callback_context_ = uninitialized_handler;
    current_node_context_     = uninitialized_destination;
    current_epoch_context_    = no_epoch;

    #if backend_check_enabled(trace_enabled)
      current_trace_context_  = trace::no_trace_event;
    #endif
  } else {
    if (insert) {
      auto iter = pending_handler_msgs_.find(handler);
      if (iter == pending_handler_msgs_.end()) {
        pending_handler_msgs_.emplace(
          std::piecewise_construct,
          std::forward_as_tuple(handler),
          std::forward_as_tuple(MsgContType{BufferedMsgType{msg,from_node}})
        );
      } else {
        iter->second.push_back(BufferedMsgType{msg,from_node});
      }
      if (!is_term || backend_check_enabled(print_term_msgs)) {
        debug_print(
          active, node,
          "deliverActiveMsg: inserting han={}, msg={}, ref={}, list size={}\n",
          handler, print_ptr(msg), envelopeGetRef(msg->env),
          pending_handler_msgs_.find(handler)->second.size()
        );
      }
      messageRef(msg);
    }
  }

  if (has_action_handler) {
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
    messageDeref(msg);
  }

  return has_action_handler;
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

    char* buf = static_cast<char*>(thePool()->alloc(num_probe_bytes));

    NodeType const& sender = stat.MPI_SOURCE;

    MPI_Recv(
      buf, num_probe_bytes, MPI_BYTE, sender, stat.MPI_TAG,
      theContext()->getComm(), MPI_STATUS_IGNORE
    );

    MessageType msg = reinterpret_cast<MessageType>(buf);
    messageConvertToShared(msg);

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
          put_tag, sender, [=](RDMA_GetType ptr, ActionType deleter){
            envelopeSetPutPtr(msg->env, std::get<0>(ptr), std::get<1>(ptr));
            handleActiveMsg(msg, sender, num_probe_bytes, true);
          }
        );
      }
    }

    if (!is_put || put_finished) {
      handleActiveMsg(msg, sender, msg_bytes, true);
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
          print_ptr(cur->buffered_msg), cur->from_node
        );
        if (deliverActiveMsg(cur->buffered_msg, cur->from_node, false)) {
          messageDeref(cur->buffered_msg);
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

HandlerType ActiveMessenger::getCurrentCallback() const {
  return current_callback_context_;
}

EpochType ActiveMessenger::getCurrentEpoch() const {
  return current_epoch_context_;
}

void ActiveMessenger::setGlobalEpoch(EpochType const& epoch) {
  global_epoch_ = epoch;
}

EpochType ActiveMessenger::getGlobalEpoch() const {
  return global_epoch_;
}

}} // end namespace vt::messaging
