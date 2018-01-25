
#include "config.h"
#include "messaging/active.h"
#include "messaging/payload.h"
#include "termination/term_headers.h"

namespace vt {

EventType ActiveMessenger::basicSendData(
  NodeType const& dest, BaseMessage* const base_msg, int const& msg_size,
  bool const& is_shared, bool const& is_term, EpochType const& epoch,
  TagType const& send_tag, EventRecordType* parent_event, ActionType next_action
) {
  auto const& this_node = theContext()->getNode();

  auto const event_id = theEvent()->createMPIEvent(this_node);
  auto& holder = theEvent()->getEventHolder(event_id);
  auto mpi_event = holder.get_event();

  auto msg = reinterpret_cast<MessageType const>(base_msg);

  if (is_shared) {
    mpi_event->setManagedMessage(msg);
  }

  if (not is_term) {
    theTerm()->produce(epoch);
  }

  MPI_Isend(
    msg, msg_size, MPI_BYTE, dest, send_tag, theContext()->getComm(),
    mpi_event->getRequest()
  );

  if (parent_event) {
    parent_event->addEventToList(event_id);
  } else {
    holder.attachAction(next_action);
  }

  if (is_shared) {
    messageDeref(msg);
  }

  return event_id;
}

EventType ActiveMessenger::sendDataDirect(
  HandlerType const& han, BaseMessage* const msg_base, int const& msg_size,
  ActionType next_action
) {
  auto const& this_node = theContext()->getNode();
  auto const& send_tag = static_cast<MPI_TagType>(MPITag::ActiveMsgTag);

  auto msg = reinterpret_cast<MessageType const>(msg_base);

  auto const& dest = envelopeGetDest(msg->env);
  auto const& is_bcast = envelopeIsBcast(msg->env);
  auto const& is_term = envelopeIsTerm(msg->env);
  auto const& is_put = envelopeIsPut(msg->env);
  auto const& epoch = envelopeIsEpochType(msg->env) ?
    envelopeGetEpoch(msg->env) : term::any_epoch_sentinel;
  auto const& is_shared = isSharedMessage(msg);

  backend_enable_if(
    trace_enabled, {
      auto const& handler = envelopeGetHandler(msg->env);
      bool const& is_auto = HandlerManagerType::isHandlerAuto(handler);
      if (is_auto) {
        trace::TraceEntryIDType ep = auto_registry::theTraceID(handler);
        if (not is_bcast) {
          trace::TraceEventIDType event = theTrace()->messageCreation(ep, msg_size);
          envelopeSetTraceEvent(msg->env, event);
        } else if (is_bcast and dest == this_node) {
          trace::TraceEventIDType event = theTrace()->messageCreationBcast(
            ep, msg_size
          );
          envelopeSetTraceEvent(msg->env, event);
        }
      }
    }
  );

  assert(
    (!is_bcast || !is_put) && "A put message cannot be a broadcast"
  );

  debug_print(
    active, node,
    "sendMsgDirect: dest=%d, handler=%d, is_bcast=%s\n",
    dest, envelopeGetHandler(msg->env), print_bool(is_bcast)
  );

  if (not is_bcast) {
    // non-broadcast message send

    if (is_put) {
      auto const put_parent_event_id = theEvent()->createParentEvent(this_node);
      auto& put_parent_holder = theEvent()->getEventHolder(put_parent_event_id);
      auto put_parent_event = put_parent_holder.get_event();

      if (next_action != nullptr) {
        put_parent_holder.attachAction(next_action);
      }

      auto const& put_ptr = envelopeGetPutPtr(msg->env);
      auto const& put_size = envelopeGetPutSize(msg->env);
      assert(
        (!(put_size != 0) || put_ptr) && "Must have valid ptr if size > 0"
      );
      auto const& ret = sendData(
        RDMA_GetType{put_ptr, put_size}, dest, no_tag, nullptr
      );
      auto const& ret_tag = std::get<1>(ret);
      auto const& put_event_send = std::get<0>(ret);
      envelopeSetPutPtr(msg->env, nullptr, static_cast<size_t>(ret_tag));

      basicSendData(
        dest, msg_base, msg_size, is_shared, is_term, epoch, send_tag,
        put_parent_event, next_action
      );

      if (next_action != nullptr) {
        put_parent_event->addEventToList(put_event_send);
      }

      return put_parent_event_id;
    } else {
      return basicSendData(
        dest, msg_base, msg_size, is_shared, is_term, epoch, send_tag,
         nullptr, next_action
      );
    }
  } else {
    // broadcast message send
    auto const& num_nodes = theContext()->getNumNodes();
    auto const& rel_node = (this_node + num_nodes - dest) % num_nodes;
    auto const& abs_child1 = rel_node*2 + 1;
    auto const& abs_child2 = rel_node*2 + 2;
    auto const& child1 = (abs_child1 + dest) % num_nodes;
    auto const& child2 = (abs_child2 + dest) % num_nodes;

    debug_print(
      active, node,
      "broadcastMsg: msg=%p, is_shared=%s, refs=%d\n",
      msg, print_bool(is_shared), envelopeGetRef(msg->env)
    );

    debug_print(
      active, node,
      "broadcastMsg: rel_node=%d, child=[%d,%d], root=%d, nodes=%d, handler=%d\n",
      rel_node, child1, child2, dest, num_nodes, envelopeGetHandler(msg->env)
    );

    if (abs_child1 >= num_nodes && abs_child2 >= num_nodes) {
      if (next_action != nullptr) {
        next_action();
      }
      return no_event;
    }

    auto const parent_event_id = theEvent()->createParentEvent(this_node);
    auto& parent_holder = theEvent()->getEventHolder(parent_event_id);
    auto parent_event = parent_holder.get_event();

    if (next_action != nullptr) {
      parent_holder.attachAction(next_action);
    }

    if (abs_child1 < num_nodes) {
      auto const event_id1 = theEvent()->createMPIEvent(this_node);
      auto& holder1 = theEvent()->getEventHolder(event_id1);
      auto mpi_event1 = holder1.get_event();

      if (is_shared) {
        mpi_event1->setManagedMessage(msg);
      }

      debug_print(
        active, node,
        "broadcastMsg: sending to c1=%d, c2=%d, bcast_root=%d, handler=%d, "
        "event_id=%lld\n",
        child1, child2, dest, envelopeGetHandler(msg->env), event_id1
      );

      if (not is_term) {
        theTerm()->produce(epoch);
      }

      MPI_Isend(
        msg, msg_size, MPI_BYTE, child1, send_tag, theContext()->getComm(),
        mpi_event1->getRequest()
      );

      parent_event->addEventToList(event_id1);
    }

    if (abs_child2 < num_nodes) {
      auto const event_id2 = theEvent()->createMPIEvent(this_node);
      auto& holder2 = theEvent()->getEventHolder(event_id2);
      auto mpi_event2 = holder2.get_event();

      if (is_shared) {
        mpi_event2->setManagedMessage(msg);
      }

      debug_print(
        active, node,
        "broadcastMsg: sending to c2=%d, c1=%d, bcast_root=%d, handler=%d, "
        "event_id=%lld\n",
        child2, child1, dest, envelopeGetHandler(msg->env), event_id2
      );

      if (not is_term) {
        theTerm()->produce(epoch);
      }

      MPI_Isend(
        msg, msg_size, MPI_BYTE, child2, send_tag, theContext()->getComm(),
        mpi_event2->getRequest()
      );

      parent_event->addEventToList(event_id2);
    }

    if (is_shared) {
      messageDeref(msg);
    }

    return parent_event_id;
  }
}

ActiveMessenger::SendDataRetType ActiveMessenger::sendData(
  RDMA_GetType const& ptr, NodeType const& dest, TagType const& tag,
  ActionType next_action
) {
  auto const& this_node = theContext()->getNode();

  auto const& data_ptr = std::get<0>(ptr);
  auto const& num_bytes = std::get<1>(ptr);
  auto const send_tag = tag == no_tag ? cur_direct_buffer_tag_++ : tag;

  auto const event_id = theEvent()->createMPIEvent(this_node);
  auto& holder = theEvent()->getEventHolder(event_id);
  auto mpi_event = holder.get_event();

  debug_print(
    active, node,
    "sendData: ptr=%p, num_bytes=%lld dest=%d, tag=%d, send_tag=%d\n",
    data_ptr, num_bytes, dest, tag, send_tag
  );

  MPI_Isend(
    data_ptr, num_bytes, MPI_BYTE, dest, send_tag, theContext()->getComm(),
    mpi_event->getRequest()
  );

  theTerm()->produce(term::any_epoch_sentinel);

  if (next_action != nullptr) {
    holder.attachAction(next_action);
  }

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
        auto const& this_node = theContext()->getNode();

        debug_print(
          active, node,
          "recvDataMsgBuffer: continuation user_buf=%p, buf=%p, tag=%d\n",
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
      "recvDataMsgBuffer: node=%d, tag=%d, enqueue=%s\n",
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

NodeType ActiveMessenger::getFromNodeCurrentHandler() {
  return current_node_context_;
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

  ActiveClosureFnType active_fun = nullptr;

  bool const& is_auto = HandlerManagerType::isHandlerAuto(handler);
  bool const& is_functor = HandlerManagerType::isHandlerFunctor(handler);

  backend_enable_if(
    trace_enabled,
    trace::TraceEntryIDType trace_id = trace::no_trace_entry_id;
    trace::TraceEventIDType trace_event = trace::no_trace_event;
  );

  debug_print(
    active, node,
    "deliverActiveMsg: msg=%p, ref=%d, is_bcast=%s\n",
    msg, envelopeGetRef(msg->env), print_bool(is_bcast)
  );

  debug_print(
    active, node,
    "deliverActiveMsg: handler=%d, is_auto=%s, is_functor=%s\n",
    handler, print_bool(is_auto), print_bool(is_functor)
  );

  if (is_auto and is_functor) {
    active_fun = auto_registry::getAutoHandlerFunctor(handler);
  } else if (is_auto) {
    active_fun = auto_registry::getAutoHandler(handler);
  } else {
    active_fun = theRegistry()->getHandler(handler, tag);
  }

  backend_enable_if(
    trace_enabled,
    if (is_auto) {
      trace_id = auto_registry::theTraceID(handler);
    }
  );

  bool const& has_action_handler = active_fun != no_action;

  if (has_action_handler) {
    // set the current handler so the user can request it in the context of an
    // active fun
    current_handler_context_ = handler;
    current_callback_context_ = callback;
    current_node_context_ = from_node;

    backend_enable_if(
      trace_enabled,
      trace_event = envelopeGetTraceEvent(msg->env);
    );

    // begin trace of this active message
    backend_enable_if(
      trace_enabled,
      theTrace()->beginProcessing(trace_id, sizeof(*msg), trace_event, from_node);
    );

    // run the active function
    active_fun(msg);

    // end trace of this active message
    backend_enable_if(
      trace_enabled,
      theTrace()->endProcessing(trace_id, sizeof(*msg), trace_event, from_node);
    );

    auto trigger = theRegistry()->getTrigger(handler);
    if (trigger) {
      trigger(msg);
    }

    // unset current handler
    current_handler_context_ = uninitialized_handler;
    current_callback_context_ = uninitialized_handler;
    current_node_context_ = uninitialized_destination;
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
    }
  }

  if (not is_term and has_action_handler) {
    theTerm()->consume(epoch);
  }

  if (has_action_handler) {
    debug_print(
      active, node,
      "deliverActiveMsg: deref msg=%p, ref=%d, is_bcast=%s, dest=%d\n",
      msg, envelopeGetRef(msg->env), print_bool(is_bcast), dest
    );
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

    NodeType const& msg_from_node = stat.MPI_SOURCE;

    MPI_Recv(
      buf, num_probe_bytes, MPI_BYTE, msg_from_node, stat.MPI_TAG,
      theContext()->getComm(), MPI_STATUS_IGNORE
    );

    MessageType msg = reinterpret_cast<MessageType>(buf);

    messageConvertToShared(msg);

    auto const& handler = envelopeGetHandler(msg->env);
    auto const& is_bcast = envelopeIsBcast(msg->env);
    auto const& dest = envelopeGetDest(msg->env);
    auto const& is_put = envelopeIsPut(msg->env);
    auto const& this_node = theContext()->getNode();

    if (is_put) {
      auto size_as_put_tag = envelopeGetPutSize(msg->env);
      TagType put_tag = static_cast<TagType>(size_as_put_tag);
      messageRef(msg);
      recvDataMsg(put_tag, msg_from_node, [=](RDMA_GetType ptr, ActionType deleter){
        envelopeSetPutPtr(msg->env, std::get<0>(ptr), std::get<1>(ptr));
        deliverActiveMsg(msg, msg_from_node, true);
        messageDeref(msg);
      });
    }

    if (is_bcast) {
      messageRef(msg);
      sendDataDirect(handler, msg, num_probe_bytes);
    }

    if ((not is_bcast or (is_bcast and dest != this_node)) and not is_put) {
      deliverActiveMsg(msg, msg_from_node, true);
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
  for (auto&& x : maybe_ready_tag_han_) {
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
  theRegistry()->swapHandler(han, fn, tag);

  if (fn != nullptr) {
    maybe_ready_tag_han_.push_back(ReadyHanTagType{han,tag});
  }
}

void ActiveMessenger::deliverPendingMsgsHandler(
  HandlerType const& han, TagType const& tag
) {
  auto iter = pending_handler_msgs_.find(han);
  if (iter != pending_handler_msgs_.end()) {
    if (iter->second.size() > 0) {
      for (auto cur = iter->second.begin(); cur != iter->second.end(); ++cur) {
        if (deliverActiveMsg(cur->buffered_msg, cur->from_node, false)) {
          cur = iter->second.erase(cur);
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
  swapHandlerFn(han, fn, tag);

  if (fn != nullptr) {
    maybe_ready_tag_han_.push_back(ReadyHanTagType{han,tag});
  }
}

void ActiveMessenger::unregisterHandlerFn(
  HandlerType const& han, TagType const& tag
) {
  return theRegistry()->unregisterHandlerFn(han, tag);
}

HandlerType ActiveMessenger::getCurrentHandler() {
  return current_handler_context_;
}

HandlerType ActiveMessenger::getCurrentCallback() {
  return current_callback_context_;
}

} //end namespace vt
