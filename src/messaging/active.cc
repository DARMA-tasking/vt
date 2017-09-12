
#include "common.h"
#include "active.h"
#include "termination.h"

namespace runtime {

EventType
ActiveMessenger::send_msg_direct(
  HandlerType const& han, BaseMessage* const msg_base, int const& msg_size,
  ActionType next_action
) {
  auto const& this_node = the_context->get_node();
  auto const& send_tag = static_cast<mpi_tag_t>(MPITag::ActiveMsgTag);

  auto msg = reinterpret_cast<message_t const>(msg_base);

  auto const& dest = envelope_get_dest(msg->env);
  auto const& is_bcast = envelope_is_bcast(msg->env);
  auto const& is_term = envelope_is_term(msg->env);
  auto const& epoch =
    envelope_is_epoch_type(msg->env) ? envelope_get_epoch(msg->env) : no_epoch;
  auto const& is_shared = is_shared_message(msg);

  backend_enable_if(
    trace_enabled, {
      auto const& handler = envelope_get_handler(msg->env);
      bool const& is_auto = handler_manager_t::is_handler_auto(handler);
      if (is_auto) {
        trace::TraceEntryIDType ep = auto_registry::get_trace_id(handler);
        if (not is_bcast) {
          trace::TraceEventIDType event = the_trace->message_creation(ep, msg_size);
          envelope_set_trace_event(msg->env, event);
        } else if (is_bcast and dest == this_node) {
          trace::TraceEventIDType event = the_trace->message_creation_bcast(
            ep, msg_size
          );
          envelope_set_trace_event(msg->env, event);
        }
      }
    }
  );

  debug_print(
    active, node,
    "send_msg_direct: dest=%d, handler=%d, is_bcast=%s\n",
    dest, envelope_get_handler(msg->env), print_bool(is_bcast)
  );

  if (not is_bcast) {
    // non-broadcast message send

    auto const event_id = the_event->create_mpi_event_id(this_node);
    auto& holder = the_event->get_event_holder(event_id);
    MPIEvent& mpi_event = *static_cast<MPIEvent*>(holder.get_event());

    if (is_shared) {
      mpi_event.set_managed_message(msg);
    }

    if (not is_term) {
      the_term->produce(epoch);
    }

    MPI_Isend(
      msg, msg_size, MPI_BYTE, dest, send_tag, MPI_COMM_WORLD,
      mpi_event.get_request()
    );

    if (next_action != nullptr) {
      holder.attach_action(next_action);
    }

    if (is_shared) {
      message_deref(msg);
    }

    return event_id;
  } else {
    // broadcast message send

    auto const& child1 = (this_node - dest)*2+1;
    auto const& child2 = (this_node - dest)*2+2;

    auto const& num_nodes = the_context->get_num_nodes();

    if (child1 >= num_nodes && child2 >= num_nodes) {
      if (next_action != nullptr) {
        next_action();
      }
      return no_event;
    }

    debug_print(
      active, node,
      "%d: broadcast_msg: child1=%d, child2=%d, broadcast_root=%d, num_nodes=%d\n",
      this_node, child1, child2, dest, num_nodes
    );

    auto const parent_event_id = the_event->create_parent_event_id(this_node);
    auto& parent_holder = the_event->get_event_holder(parent_event_id);
    ParentEvent& parent_event = *static_cast<ParentEvent*>(parent_holder.get_event());

    if (next_action != nullptr) {
      parent_holder.attach_action(next_action);
    }

    if (child1 < num_nodes) {
      auto const event_id1 = the_event->create_mpi_event_id(this_node);
      auto& holder1 = the_event->get_event_holder(event_id1);
      MPIEvent& mpi_event1 = *static_cast<MPIEvent*>(holder1.get_event());

      if (is_shared) {
        mpi_event1.set_managed_message(msg);
      }

      debug_print(
        active, node,
        "broadcast_msg: sending to child1=%d, child2=%d, broadcast_root=%d, "
        "event_id=%lld\n",
        child1, child2, dest, event_id1
      );

      if (not is_term) {
        the_term->produce(epoch);
      }

      MPI_Isend(
        msg, msg_size, MPI_BYTE, child1, send_tag, MPI_COMM_WORLD,
        mpi_event1.get_request()
      );

      parent_event.add_event(event_id1);
    }

    if (child2 < num_nodes) {
      auto const event_id2 = the_event->create_mpi_event_id(this_node);
      auto& holder2 = the_event->get_event_holder(event_id2);
      MPIEvent& mpi_event2 = *static_cast<MPIEvent*>(holder2.get_event());

      if (is_shared) {
        mpi_event2.set_managed_message(msg);
      }

      debug_print(
        active, node,
        "broadcast_msg: sending to child2=%d, child1=%d, broadcast_root=%d, "
        "event_id=%lld\n",
        child2, child1, dest, event_id2
      );

      if (not is_term) {
        the_term->produce(epoch);
      }

      MPI_Isend(
        msg, msg_size, MPI_BYTE, child2, send_tag, MPI_COMM_WORLD,
        mpi_event2.get_request()
      );

      parent_event.add_event(event_id2);
    }

    if (is_shared) {
      message_deref(msg);
    }

    return parent_event_id;
  }
}

ActiveMessenger::send_data_ret_t
ActiveMessenger::send_data(
  RDMA_GetType const& ptr, NodeType const& dest, TagType const& tag,
  ActionType next_action
) {
  auto const& this_node = the_context->get_node();

  auto const& data_ptr = std::get<0>(ptr);
  auto const& num_bytes = std::get<1>(ptr);
  auto const send_tag = tag == no_tag ? cur_direct_buffer_tag++ : tag;

  auto const event_id = the_event->create_mpi_event_id(this_node);
  auto& holder = the_event->get_event_holder(event_id);
  MPIEvent& mpi_event = *static_cast<MPIEvent*>(holder.get_event());

  debug_print(
    active, node,
    "%d: send_data: ptr=%p, num_bytes=%lld dest=%d, tag=%d, send_tag=%d\n",
    this_node, data_ptr, num_bytes, dest, tag, send_tag
  );

  MPI_Isend(
    data_ptr, num_bytes, MPI_BYTE, dest, send_tag, MPI_COMM_WORLD,
    mpi_event.get_request()
  );

  the_term->produce(no_epoch);

  if (next_action != nullptr) {
    holder.attach_action(next_action);
  }

  return send_data_ret_t{event_id,send_tag};
}

bool
ActiveMessenger::recv_data_msg(
  TagType const& tag, NodeType const& node, RDMA_ContinuationDeleteType next
) {
  return recv_data_msg(tag, node, true, next);
}

bool
ActiveMessenger::process_data_msg_recv() {
  bool erase = false;
  auto iter = pending_recvs.begin();

  for (; iter != pending_recvs.end(); ++iter) {
    auto const& done = recv_data_msg_buffer(
      iter->second.user_buf, iter->first, iter->second.recv_node,
      false, iter->second.dealloc_user_buf, iter->second.cont
    );
    if (done) {
      erase = true;
      break;
    }
  }

  if (erase) {
    pending_recvs.erase(iter);
    return true;
  } else {
    return false;
  }
}

bool
ActiveMessenger::recv_data_msg_buffer(
  void* const user_buf, TagType const& tag, NodeType const& node,
  bool const& enqueue, ActionType dealloc_user_buf, RDMA_ContinuationDeleteType next
) {
  if (not enqueue) {
    CountType num_probe_bytes;
    MPI_Status stat;
    int flag;

    MPI_Iprobe(
      node == uninitialized_destination ? MPI_ANY_SOURCE : node,
      tag, MPI_COMM_WORLD, &flag, &stat
    );

    if (flag == 1) {
      MPI_Get_count(&stat, MPI_BYTE, &num_probe_bytes);

      char* buf =
        user_buf == nullptr ?
        static_cast<char*>(the_pool->alloc(num_probe_bytes)) :
        static_cast<char*>(user_buf);

      MPI_Recv(
        buf, num_probe_bytes, MPI_BYTE, stat.MPI_SOURCE, stat.MPI_TAG,
        MPI_COMM_WORLD, MPI_STATUS_IGNORE
      );

      auto dealloc_buf = [=]{
        auto const& this_node = the_context->get_node();

        debug_print(
          active, node,
          "%d: recv_data_msg_buffer: continuation user_buf=%p, buf=%p, tag=%d\n",
          this_node, user_buf, buf, tag
        );

        if (user_buf == nullptr) {
          the_pool->dealloc(buf);
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

      the_term->consume(no_epoch);

      return true;
    } else {
      return false;
    }
  } else {
    debug_print(
      active, node,
      "recv_data_msg_buffer: node=%d, tag=%d, enqueue=%s\n",
      node, tag, print_bool(enqueue)
    );

    pending_recvs.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(tag),
      std::forward_as_tuple(pending_recv_t{user_buf,next,dealloc_user_buf,node})
    );
    return false;
  }
}

bool
ActiveMessenger::recv_data_msg(
  TagType const& tag, NodeType const& recv_node, bool const& enqueue,
  RDMA_ContinuationDeleteType next
) {
  return recv_data_msg_buffer(nullptr, tag, recv_node, enqueue, nullptr, next);
}

NodeType
ActiveMessenger::get_from_node_current_handler() {
  return current_node_context;
}

bool
ActiveMessenger::deliver_active_msg(
  message_t msg, NodeType const& in_from_node, bool insert
) {
  auto const& is_term = envelope_is_term(msg->env);
  auto const& is_bcast = envelope_is_bcast(msg->env);
  auto const& dest = envelope_get_dest(msg->env);
  auto const& handler = envelope_get_handler(msg->env);
  auto const& epoch =
    envelope_is_epoch_type(msg->env) ? envelope_get_epoch(msg->env) : no_epoch;
  auto const& is_tag = envelope_is_tag_type(msg->env);
  auto const& tag = is_tag ? envelope_get_tag(msg->env) : no_tag;
  auto const& callback =
    envelope_is_callback_type(msg->env) ?
    get_callback_message(msg) : uninitialized_handler;
  auto const& from_node = is_bcast ? dest : in_from_node;

  active_function_t active_fun = nullptr;

  bool const& is_auto = handler_manager_t::is_handler_auto(handler);
  bool const& is_functor = handler_manager_t::is_handler_functor(handler);

  backend_enable_if(
    trace_enabled,
    trace::TraceEntryIDType trace_id = trace::no_trace_entry_id;
    trace::TraceEventIDType trace_event = trace::no_trace_event;
  );

  debug_print(
    active, node,
    "deliver_active_msg: handler=%d, is_auto=%s, is_functor=%s\n",
    handler, print_bool(is_auto), print_bool(is_functor)
  );

  if (is_auto and is_functor) {
    active_fun = auto_registry::get_auto_handler_functor(handler);
  } else if (is_auto) {
    active_fun = auto_registry::get_auto_handler(handler);
  } else {
    active_fun = the_registry->get_handler(handler, tag);
  }

  backend_enable_if(
    trace_enabled,
    if (is_auto) {
      trace_id = auto_registry::get_trace_id(handler);
    }
  );

  bool const& has_action_handler = active_fun != no_action;

  if (has_action_handler) {
    // set the current handler so the user can request it in the context of an
    // active fun
    current_handler_context = handler;
    current_callback_context = callback;
    current_node_context = from_node;

    backend_enable_if(
      trace_enabled,
      trace_event = envelope_get_trace_event(msg->env);
    );

    // begin trace of this active message
    backend_enable_if(
      trace_enabled,
      the_trace->begin_processing(trace_id, sizeof(*msg), trace_event, from_node);
    );

    // run the active function
    active_fun(msg);

    // end trace of this active message
    backend_enable_if(
      trace_enabled,
      the_trace->end_processing(trace_id, sizeof(*msg), trace_event, from_node);
    );

    auto trigger = the_registry->get_trigger(handler);
    if (trigger) {
      trigger(msg);
    }

    // unset current handler
    current_handler_context = uninitialized_handler;
    current_callback_context = uninitialized_handler;
    current_node_context = uninitialized_destination;
  } else {
    if (insert) {
      auto iter = pending_handler_msgs.find(handler);
      if (iter == pending_handler_msgs.end()) {
        pending_handler_msgs.emplace(
          std::piecewise_construct,
          std::forward_as_tuple(handler),
          std::forward_as_tuple(msg_cont_t{buffered_msg_t{msg,from_node}})
        );
      } else {
        iter->second.push_back(buffered_msg_t{msg,from_node});
      }
    }
  }

  if (not is_term and has_action_handler) {
    the_term->consume(epoch);
  }

  if (not is_bcast and has_action_handler) {
    message_deref(msg);
  }

  return has_action_handler;
}

bool
ActiveMessenger::try_process_incoming_message() {
  CountType num_probe_bytes;
  MPI_Status stat;
  int flag;

  MPI_Iprobe(
    MPI_ANY_SOURCE, static_cast<mpi_tag_t>(MPITag::ActiveMsgTag),
    MPI_COMM_WORLD, &flag, &stat
  );

  if (flag == 1) {
    MPI_Get_count(&stat, MPI_BYTE, &num_probe_bytes);

    char* buf = static_cast<char*>(the_pool->alloc(num_probe_bytes));

    NodeType const& msg_from_node = stat.MPI_SOURCE;

    MPI_Recv(
      buf, num_probe_bytes, MPI_BYTE, msg_from_node, stat.MPI_TAG,
      MPI_COMM_WORLD, MPI_STATUS_IGNORE
    );

    message_t msg = reinterpret_cast<message_t>(buf);

    message_convert_to_shared(msg);

    auto const& handler = envelope_get_handler(msg->env);
    auto const& is_bcast = envelope_is_bcast(msg->env);

    if (is_bcast) {
      send_msg_direct(handler, msg, num_probe_bytes);
    }

    deliver_active_msg(msg, msg_from_node, true);

    return true;
  } else {
    return false;
  }
}

bool
ActiveMessenger::scheduler() {
  bool const processed = try_process_incoming_message();
  bool const processed_data_msg = process_data_msg_recv();
  process_maybe_ready_han_tag();

  return processed or processed_data_msg;
}

bool
ActiveMessenger::is_local_term() {
  bool const no_pending_msgs = pending_handler_msgs.size() == 0;
  bool const no_pending_recvs = pending_recvs.size() == 0;
  return no_pending_msgs and no_pending_recvs;
}

void
ActiveMessenger::process_maybe_ready_han_tag() {
  for (auto&& x : maybe_ready_tag_han) {
    deliver_pending_msgs_on_han(std::get<0>(x), std::get<1>(x));
  }
}

HandlerType
ActiveMessenger::register_new_handler(active_function_t fn, TagType const& tag) {
  return the_registry->register_new_handler(fn, tag);
}

HandlerType
ActiveMessenger::collective_register_handler(
  active_function_t fn, TagType const& tag
) {
  return the_registry->register_active_handler(fn, tag);
}

void
ActiveMessenger::swap_handler_fn(
  HandlerType const& han, active_function_t fn, TagType const& tag
) {
  the_registry->swap_handler(han, fn, tag);

  if (fn != nullptr) {
    maybe_ready_tag_han.push_back(ready_han_tag_t{han,tag});
  }
}

void
ActiveMessenger::deliver_pending_msgs_on_han(
  HandlerType const& han, TagType const& tag
) {
  auto iter = pending_handler_msgs.find(han);
  if (iter != pending_handler_msgs.end()) {
    if (iter->second.size() > 0) {
      for (auto cur = iter->second.begin(); cur != iter->second.end(); ++cur) {
        if (deliver_active_msg(cur->buffered_msg, cur->from_node, false)) {
          cur = iter->second.erase(cur);
        }
      }
    } else {
      pending_handler_msgs.erase(iter);
    }
  }
}

void
ActiveMessenger::register_handler_fn(
  HandlerType const& han, active_function_t fn, TagType const& tag
) {
  swap_handler_fn(han, fn, tag);

  if (fn != nullptr) {
    maybe_ready_tag_han.push_back(ready_han_tag_t{han,tag});
  }
}

void
ActiveMessenger::unregister_handler_fn(
  HandlerType const& han, TagType const& tag
) {
  return the_registry->unregister_handler_fn(han, tag);
}

HandlerType
ActiveMessenger::get_current_handler() {
  return current_handler_context;
}

HandlerType
ActiveMessenger::get_current_callback() {
  return current_callback_context;
}

} //end namespace runtime
