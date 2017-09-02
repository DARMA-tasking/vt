
#include "common.h"
#include "active.h"
#include "termination.h"

namespace runtime {

event_t
ActiveMessenger::send_msg_direct(
  handler_t const& han, BaseMessage* const msg_base, int const& msg_size,
  action_t next_action
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

    debug_print_active(
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

      debug_print_active(
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

      debug_print_active(
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

void
ActiveMessenger::check_term_single_node() {
  auto const& num_nodes = the_context->get_num_nodes();
  if (num_nodes == 1) {
    the_term->maybe_propagate();
  }
}

ActiveMessenger::send_data_ret_t
ActiveMessenger::send_data(
  rdma_get_t const& ptr, node_t const& dest, tag_t const& tag,
  action_t next_action
) {
  auto const& this_node = the_context->get_node();

  auto const& data_ptr = std::get<0>(ptr);
  auto const& num_bytes = std::get<1>(ptr);
  auto const send_tag = tag == no_tag ? cur_direct_buffer_tag++ : tag;

  auto const event_id = the_event->create_mpi_event_id(this_node);
  auto& holder = the_event->get_event_holder(event_id);
  MPIEvent& mpi_event = *static_cast<MPIEvent*>(holder.get_event());

  debug_print_active(
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
  tag_t const& tag, node_t const& node, rdma_continuation_del_t next
) {
  return recv_data_msg(tag, node, true, next);
}

void
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
  }
}

bool
ActiveMessenger::recv_data_msg_buffer(
  void* const user_buf, tag_t const& tag, node_t const& node,
  bool const& enqueue, action_t dealloc_user_buf, rdma_continuation_del_t next
) {
  if (not enqueue) {
    byte_t num_probe_bytes;
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

        debug_print_active(
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
        next(rdma_get_t{buf,num_probe_bytes}, [=]{
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
    debug_print_active(
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
  tag_t const& tag, node_t const& recv_node, bool const& enqueue,
  rdma_continuation_del_t next
) {
  return recv_data_msg_buffer(nullptr, tag, recv_node, enqueue, nullptr, next);
}

bool
ActiveMessenger::deliver_active_msg(message_t msg, bool insert) {
  auto const& is_term = envelope_is_term(msg->env);
  auto const& is_bcast = envelope_is_bcast(msg->env);
  auto const& handler = envelope_get_handler(msg->env);
  auto const& epoch =
    envelope_is_epoch_type(msg->env) ? envelope_get_epoch(msg->env) : no_epoch;
  auto const& is_tag = envelope_is_tag_type(msg->env);
  auto const& tag = is_tag ? envelope_get_tag(msg->env) : no_tag;
  auto const& callback =
    envelope_is_callback_type(msg->env) ?
    get_callback_message(msg) : uninitialized_handler;

  active_function_t active_fun = nullptr;

  if (handler_manager_t::is_handler_auto(handler)) {
    active_fun = auto_registry::get_auto_handler(msg);
  } else {
    active_fun = the_registry->get_handler(handler, tag);
  }

  bool const& has_action_handler = active_fun != no_action;

  if (has_action_handler) {
    // set the current handler so the user can request it in the context of an
    // active fun
    current_handler_context = handler;
    current_callback_context = callback;

    // run the active function
    active_fun(msg);

    auto trigger = the_registry->get_trigger(handler);
    if (trigger) {
      trigger(msg);
    }

    // unset current handler
    current_handler_context = uninitialized_handler;
    current_callback_context = uninitialized_handler;
  } else {
    if (insert) {
      auto iter = pending_handler_msgs.find(handler);
      if (iter == pending_handler_msgs.end()) {
        pending_handler_msgs.emplace(
          std::piecewise_construct,
          std::forward_as_tuple(handler),
          std::forward_as_tuple(msg_cont_t{msg})
        );
      } else {
        iter->second.push_back(msg);
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
  byte_t num_probe_bytes;
  MPI_Status stat;
  int flag;

  MPI_Iprobe(
    MPI_ANY_SOURCE, static_cast<mpi_tag_t>(MPITag::ActiveMsgTag),
    MPI_COMM_WORLD, &flag, &stat
  );

  if (flag == 1) {
    MPI_Get_count(&stat, MPI_BYTE, &num_probe_bytes);

    char* buf = static_cast<char*>(the_pool->alloc(num_probe_bytes));

    MPI_Recv(
      buf, num_probe_bytes, MPI_BYTE, stat.MPI_SOURCE, stat.MPI_TAG,
      MPI_COMM_WORLD, MPI_STATUS_IGNORE
    );

    message_t msg = reinterpret_cast<message_t>(buf);

    message_convert_to_shared(msg);

    auto const& handler = envelope_get_handler(msg->env);
    auto const& is_bcast = envelope_is_bcast(msg->env);

    if (is_bcast) {
      send_msg_direct(handler, msg, num_probe_bytes);
    }

    deliver_active_msg(msg, true);

    return true;
  } else {
    return false;
  }
}

void
ActiveMessenger::scheduler(int const& num_times) {
  for (int i = 0; i < num_times; i++) {
    try_process_incoming_message();
    perform_triggered_actions();
    check_term_single_node();
    process_data_msg_recv();
    process_maybe_ready_han_tag();
  }
}

void
ActiveMessenger::process_maybe_ready_han_tag() {
  for (auto&& x : maybe_ready_tag_han) {
    deliver_pending_msgs_on_han(std::get<0>(x), std::get<1>(x));
  }
}

void
ActiveMessenger::perform_triggered_actions() {
  the_event->test_events_trigger(mpi_event_tag);
  the_event->test_events_trigger(normal_event_tag);
}

handler_t
ActiveMessenger::register_new_handler(active_function_t fn, tag_t const& tag) {
  return the_registry->register_new_handler(fn, tag);
}

handler_t
ActiveMessenger::collective_register_handler(
  active_function_t fn, tag_t const& tag
) {
  return the_registry->register_active_handler(fn, tag);
}

void
ActiveMessenger::swap_handler_fn(
  handler_t const& han, active_function_t fn, tag_t const& tag
) {
  the_registry->swap_handler(han, fn, tag);

  if (fn != nullptr) {
    maybe_ready_tag_han.push_back(ready_han_tag_t{han,tag});
  }
}

void
ActiveMessenger::deliver_pending_msgs_on_han(
  handler_t const& han, tag_t const& tag
) {
  auto iter = pending_handler_msgs.find(han);
  if (iter != pending_handler_msgs.end()) {
    if (iter->second.size() > 0) {
      for (auto cur = iter->second.begin(); cur != iter->second.end(); ++cur) {
        if (deliver_active_msg(*cur, false)) {
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
  handler_t const& han, active_function_t fn, tag_t const& tag
) {
  swap_handler_fn(han, fn, tag);

  if (fn != nullptr) {
    maybe_ready_tag_han.push_back(ready_han_tag_t{han,tag});
  }
}

void
ActiveMessenger::unregister_handler_fn(
  handler_t const& han, tag_t const& tag
) {
  return the_registry->unregister_handler_fn(han, tag);
}

handler_t
ActiveMessenger::get_current_handler() {
  return current_handler_context;
}

handler_t
ActiveMessenger::get_current_callback() {
  return current_callback_context;
}

} //end namespace runtime
