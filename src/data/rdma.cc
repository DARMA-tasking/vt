
#include "rdma.h"
#include "transport.h"

namespace runtime { namespace rdma {

rdma_handle_t
RDMAManager::register_new_rdma_handler(
  bool const& use_default, rdma_ptr_t const& ptr, byte_t const& num_bytes,
  bool const& is_collective
) {
  auto const& this_node = the_context->get_node();

  rdma_handler_t new_handle = rdma_handle_manager_t::create_new_handler();
  rdma_identifier_t const& new_identifier =
    is_collective ? cur_collective_ident : cur_ident++;

  bool const is_sized = false;

  debug_print_rdma(
    "register_new_rdma_handler: my_handle=%llx, op=%d\n",
    new_handle, rdma_handle_manager_t::get_op_type(new_handle)
  );

  rdma_handle_manager_t::set_op_type(new_handle, rdma_type_t::GetOrPut);
  rdma_handle_manager_t::set_is_collective(new_handle, is_collective);
  rdma_handle_manager_t::set_is_sized(new_handle, is_sized);
  rdma_handle_manager_t::set_rdma_node(new_handle, this_node);
  rdma_handle_manager_t::set_rdma_identifier(new_handle, new_identifier);

  holder.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(new_handle),
    std::forward_as_tuple(rdma_state_t{new_handle, ptr, num_bytes, use_default})
  );

  if (use_default) {
    holder.find(new_handle)->second.set_default_handler();
  }

  return new_handle;
}

void
RDMAManager::unregister_rdma_handler(
  rdma_handle_t const& han, rdma_type_t const& type, tag_t const& tag,
  bool const& use_default
) {
  auto holder_iter = holder.find(han);
  assert(
    holder_iter != holder.end() and "Holder for handler must exist here"
  );

  auto& state = holder_iter->second;
  state.unregister_rdma_handler(type, tag, use_default);
}

void
RDMAManager::unregister_rdma_handler(
  rdma_handle_t const& han, rdma_handler_t const& handler, tag_t const& tag
) {
  auto holder_iter = holder.find(han);
  assert(
    holder_iter != holder.end() and "Holder for handler must exist here"
  );

  auto& state = holder_iter->second;
  state.unregister_rdma_handler(handler, tag);
}

rdma_handler_t
RDMAManager::allocate_new_rdma_handler() {
  rdma_handler_t const handler = cur_rdma_handler++;
  return handler;
}

void
RDMAManager::request_get_data(
  GetMessage* msg, bool const& is_user_msg,
  rdma_handle_t const& han, tag_t const& tag, byte_t const& num_bytes,
  rdma_ptr_t const& ptr, rdma_continuation_t cont, action_t next_action
) {
  auto const& this_node = the_context->get_node();
  auto const handler_node = rdma_handle_manager_t::get_rdma_node(han);

  assert(
    handler_node == this_node and "Handle must be local to this node"
  );

  auto holder_iter = holder.find(han);
  assert(
    holder_iter != holder.end() and "Holder for handler must exist here"
  );

  auto& state = holder_iter->second;

  rdma_info_t info(rdma_type_t::Get, num_bytes, tag, cont, next_action, ptr);

  return state.get_data(msg, is_user_msg, info);
}

void
RDMAManager::trigger_get_recv_data(
  rdma_op_t const& op, tag_t const& tag, rdma_ptr_t ptr, byte_t const& num_bytes,
  action_t const& action
) {
  auto iter = pending_ops.find(op);

  assert(
    iter != pending_ops.end() and "Pending op must exist"
  );

  if (iter->second.cont) {
    iter->second.cont(ptr, num_bytes);
  } else {
    std::memcpy(iter->second.data_ptr, ptr, num_bytes);
  }

  pending_ops.erase(iter);

  if (action != nullptr) {
    action();
  }
}

RDMAManager::rdma_direct_t
RDMAManager::try_get_data_ptr_direct(rdma_op_t const& op) {
  auto iter = pending_ops.find(op);

  assert(
    iter != pending_ops.end() and "Pending op must exist"
  );

  if (iter->second.cont) {
    return rdma_direct_t{nullptr,nullptr};
  } else {
    auto ptr = iter->second.data_ptr;
    auto action = iter->second.cont2;
    pending_ops.erase(iter);
    assert(
      ptr != nullptr and "ptr must be set"
    );
    return rdma_direct_t{ptr,action};
  }
}

void
RDMAManager::trigger_put_back_data(rdma_op_t const& op) {
  auto iter = pending_ops.find(op);

  assert(
    iter != pending_ops.end() and "Pending op must exist"
  );

  iter->second.cont2();

  pending_ops.erase(iter);
}

void
RDMAManager::trigger_put_recv_data(
  rdma_handle_t const& han, tag_t const& tag, rdma_ptr_t ptr,
  byte_t const& num_bytes, action_t const& action
) {
  auto const& this_node = the_context->get_node();
  auto const handler_node = rdma_handle_manager_t::get_rdma_node(han);

  assert(
    handler_node == this_node and "Handle must be local to this node"
  );

  auto holder_iter = holder.find(han);
  assert(
    holder_iter != holder.end() and "Holder for handler must exist here"
  );

  auto& state = holder_iter->second;

  bool const is_user_msg = false;
  rdma_info_t info(rdma_type_t::Put, num_bytes, tag, nullptr, action, ptr);

  return state.put_data(nullptr, is_user_msg, info);
}

rdma_ptr_t
RDMAManager::try_put_ptr(
  rdma_handle_t const& han, tag_t const& tag
) {
  auto const& this_node = the_context->get_node();
  auto const handler_node = rdma_handle_manager_t::get_rdma_node(han);

  assert(
    handler_node == this_node and "Handle must be local to this node"
  );
  auto holder_iter = holder.find(han);
  assert(
    holder_iter != holder.end() and "Holder for handler must exist here"
  );

  auto& state = holder_iter->second;

  if (state.using_default_put_handler) {
    return state.ptr;
  } else {
    return nullptr;
  }
}

void
RDMAManager::send_data_channel(
  rdma_type_t const& type, rdma_handle_t const& han, rdma_ptr_t const& ptr,
  byte_t const& num_bytes, action_t cont, action_t action_after_put
) {
  rdma_handle_t ch_han = han;
  rdma_handle_manager_t::set_op_type(ch_han, type);

  auto channel_iter = channels.find(ch_han);
  assert(
    channel_iter != channels.end() and "Channel must exist"
  );
  Channel& channel = channel_iter->second;

  channel.write_data_to_channel(ptr, num_bytes);

  if (cont) {
    cont();
  }

  assert(action_after_put == nullptr);
}

void
RDMAManager::put_data(
  rdma_handle_t const& han, rdma_ptr_t const& ptr, byte_t const& num_bytes,
  tag_t const& tag, action_t cont, action_t action_after_put
) {
  auto const& this_node = the_context->get_node();
  auto const put_node = rdma_handle_manager_t::get_rdma_node(han);

  if (put_node != this_node) {
    rdma_handle_t ch_han = han;
    rdma_handle_manager_t::set_op_type(ch_han, rdma_type_t::Put);

    bool const send_via_channel =
      channels.find(ch_han) != channels.end() and tag == no_tag and
      action_after_put == nullptr;

    if (send_via_channel) {
      return send_data_channel(
        rdma_type_t::Put, han, ptr, num_bytes, cont, action_after_put
      );
    } else {
      rdma_op_t const new_op = cur_op++;

      tag_t recv_tag = no_tag;

      PutMessage* msg = new PutMessage(
        new_op, num_bytes, no_tag, han,
        action_after_put ? this_node : uninitialized_destination
      );

      auto send_payload = [&](ActiveMessenger::send_fn_t send){
        auto ret = send(rdma_get_t{ptr, num_bytes}, put_node, no_tag, [=]{
          if (cont != nullptr) {
            cont();
          }
        });
        msg->mpi_tag_to_recv = std::get<1>(ret);
      };

      debug_print_rdma(
        "%d: put_data: sending: ptr=%p, num_bytes=%lld, recv_tag=%d\n",
        this_node, ptr, num_bytes, recv_tag
      );

      set_put_type(msg->env);

      if (tag != no_tag) {
        envelope_set_tag(msg->env, tag);
      }

      auto deleter = [=]{ delete msg; };

      the_msg->send_msg(
        put_node, the_rdma->put_recv_msg_han, msg, send_payload, deleter
      );

      if (action_after_put != nullptr) {
        pending_ops.emplace(
          std::piecewise_construct,
          std::forward_as_tuple(new_op),
          std::forward_as_tuple(rdma_pending_t{action_after_put})
        );
      }
    }
  } else {
    the_rdma->trigger_put_recv_data(
      han, tag, ptr, num_bytes, [=](){
        debug_print_rdma("%d: put_data: local data is put\n", this_node);
        if (cont) {
          cont();
        }
        if (action_after_put) {
          action_after_put();
        }
      }
    );
  }

}

void
RDMAManager::get_data_info_buf(
  rdma_handle_t const& han, rdma_ptr_t const& ptr, byte_t const& num_bytes,
  tag_t const& tag, action_t next_action
) {
  auto const& this_node = the_context->get_node();
  auto const get_node = rdma_handle_manager_t::get_rdma_node(han);

  if (get_node != this_node) {
    rdma_handle_t ch_han = han;
    rdma_handle_manager_t::set_op_type(ch_han, rdma_type_t::Get);

    bool const send_via_channel =
      channels.find(ch_han) != channels.end() and tag == no_tag and
      next_action == nullptr;

    if (send_via_channel) {
      return send_data_channel(
        rdma_type_t::Get, han, ptr, num_bytes, nullptr, next_action
      );
    } else {
      rdma_op_t const new_op = cur_op++;

      GetMessage* msg = new GetMessage(new_op, this_node, han, num_bytes);
      if (tag != no_tag) {
        envelope_set_tag(msg->env, tag);
      }
      the_msg->send_msg(get_node, get_msg_han, msg, [=]{ delete msg; });

      pending_ops.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(new_op),
        std::forward_as_tuple(rdma_pending_t{ptr, next_action})
      );
    }
  } else {
    debug_print_rdma(
      "%d: get_data: local direct into buf, ptr=%p\n", this_node, ptr
    );
    the_rdma->request_get_data(
      nullptr, false, han, tag, num_bytes, ptr, nullptr, next_action
    );
  }
}

void
RDMAManager::get_data(
  rdma_handle_t const& han, tag_t const& tag, byte_t const& num_bytes,
  rdma_recv_t cont
) {
  auto const& this_node = the_context->get_node();
  auto const get_node = rdma_handle_manager_t::get_rdma_node(han);

  if (get_node != this_node) {
    rdma_op_t const new_op = cur_op++;

    GetMessage* msg = new GetMessage(new_op, this_node, han, num_bytes);
    if (tag != no_tag) {
      envelope_set_tag(msg->env, tag);
    }
    the_msg->send_msg(get_node, get_msg_han, msg, [=]{ delete msg; });

    pending_ops.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(new_op),
      std::forward_as_tuple(rdma_pending_t{cont})
    );
  } else {
    the_rdma->request_get_data(
      nullptr, false, han, tag, num_bytes, nullptr, [cont](rdma_get_t data){
        debug_print_rdma("local: data is ready\n");
        cont(std::get<0>(data), std::get<1>(data));
      }
    );
  }
}

void
RDMAManager::create_get_channel(
  rdma_handle_t const& han, action_t const& action
) {
  return create_direct_channel(rdma_type_t::Get, han, action);
}

void
RDMAManager::create_put_channel(
  rdma_handle_t const& han, action_t const& action
) {
  return create_direct_channel(rdma_type_t::Put, han, action);
}

void
RDMAManager::setup_get_channel_with_remote(
  rdma_handle_t const& han, node_t const& dest, action_t const& action
) {
  return setup_channel_with_remote(rdma_type_t::Get, han, dest, action);
}

void
RDMAManager::setup_put_channel_with_remote(
  rdma_handle_t const& han, node_t const& dest, action_t const& action
) {
  return setup_channel_with_remote(rdma_type_t::Put, han, dest, action);
}

void
RDMAManager::setup_channel_with_remote(
  rdma_type_t const& type, rdma_handle_t const& han, node_t const& dest,
  action_t const& action
) {
  auto const& this_node = the_context->get_node();
  auto const target = rdma_handle_manager_t::get_rdma_node(han);

  rdma_handle_t ch_han = han;
  rdma_handle_manager_t::set_op_type(ch_han, type);

  auto iter = channels.find(ch_han);
  if (iter == channels.end()) {
    auto const& tag = next_rdma_channel_tag();
    auto const& num_bytes = lookup_bytes_handler(han);
    auto const& other_node = target == this_node ? dest : target;

    auto msg = make_shared_message<ChannelMessage>(
      type, han, num_bytes, tag
    );

    the_msg->send_msg_callback(
      remote_channel_han, other_node, msg, [=](runtime::BaseMessage* in_msg){
        action();
      }
    );

    return create_direct_channel_internal(
      type, han, dest, nullptr, tag, num_bytes
    );
  }
}

void
RDMAManager::create_direct_channel(
  rdma_type_t const& type, rdma_handle_t const& han, action_t const& action
) {
  auto const& this_node = the_context->get_node();
  auto const target = rdma_handle_manager_t::get_rdma_node(han);
  bool const& handler_on_node = target == this_node;
  if (not handler_on_node) {
    return create_direct_channel_internal(type, han, this_node, action);
  } else {
    // do nothing
  }
}

void
RDMAManager::create_direct_channel_finish(
  rdma_type_t const& type, rdma_handle_t const& han, node_t const& non_target,
  action_t const& action, tag_t const& channel_tag, bool const& is_target,
  byte_t const& num_bytes
) {
  auto const& this_node = the_context->get_node();
  auto const target = rdma_handle_manager_t::get_rdma_node(han);

  rdma_ptr_t target_ptr = no_rdma_ptr;
  byte_t target_num_bytes = num_bytes;

  if (is_target) {
    auto holder_iter = holder.find(han);
    assert(
      holder_iter != holder.end() and "Holder for handler must exist here"
    );

    auto& state = holder_iter->second;

    target_ptr = state.ptr;
    target_num_bytes = state.num_bytes;

    debug_print_rdma_channel(
      "%d: create_direct_channel: han=%lld, is_target=%s, state ptr=%p, bytes=%lld\n",
      this_node, han, print_bool(is_target), target_ptr, target_num_bytes
    );
  }

  rdma_handle_t ch_han = han;
  rdma_handle_manager_t::set_op_type(ch_han, type);

  auto iter = channels.find(ch_han);
  if (iter == channels.end()) {
    debug_print_rdma_channel(
      "%d: create_direct_channel: han=%lld, is_target=%s, creating\n",
      this_node, han, print_bool(is_target)
    );

    // create a new rdma channel
    channels.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(ch_han),
      std::forward_as_tuple(rdma_channel_t{
        han, type, is_target, channel_tag, non_target, target_ptr,
        target_num_bytes
     })
    );

    iter = channels.find(ch_han);

    assert(
      iter != channels.end() and "Channel must exist now"
    );

    Channel& new_channel = iter->second;

    new_channel.init_channel_group();

    if (action) {
      action();
    }
  } else {
    if (action) {
      action();
    }
  }
}

void
RDMAManager::create_direct_channel_internal(
  rdma_type_t const& type, rdma_handle_t const& han, node_t const& non_target,
  action_t const& action, tag_t const& channel_tag, byte_t const& num_bytes
) {
  auto const& this_node = the_context->get_node();
  auto const target = rdma_handle_manager_t::get_rdma_node(han);
  auto const rdma_op_type = rdma_handle_manager_t::get_op_type(han);

  bool const& handler_on_node = target == this_node;

  bool const& is_get = type == rdma_type_t::Get;
  bool const& is_put = type == rdma_type_t::Put;

  assert(
    rdma_op_type == type or rdma_op_type == rdma_type_t::GetOrPut
  );

  bool is_target = false;

  if (is_put and handler_on_node) {
    is_target = true;
  } else if (is_get and handler_on_node) {
    is_target = true;
  }

  // check to see if it already exists
  rdma_handle_t ch_han = han;
  rdma_handle_manager_t::set_op_type(ch_han, type);
  if (channels.find(ch_han) != channels.end()) {
    debug_print_rdma_channel(
      "%d: create_direct_channel: han=%lld, target=%d, already created!\n",
      this_node, han, target
    );
    if (action) {
      action();
    }
    return;
  }

  debug_print_rdma_channel(
    "%d: create_direct_channel: han=%lld, target=%d, op_type=%d, is_target=%s, "
    "channel_tag=%d\n",
    this_node, han, target, rdma_op_type, print_bool(is_target), channel_tag
  );

  if (not is_target and channel_tag == no_tag and num_bytes == no_byte) {
    assert(
      channel_tag == no_tag and "Should not have tag assigned"
    );

    auto const& unique_channel_tag = next_rdma_channel_tag();

    debug_print_rdma_channel(
      "%d: create_direct_channel: generate unique tag: channel_tag=%d\n",
      this_node, unique_channel_tag
    );

    assert(
      channel_tag == no_tag and
      "Should not have a tag assigned at this point"
    );

    auto msg = make_shared_message<CreateChannel>(
      type, han, unique_channel_tag, target, this_node
    );

    the_msg->send_msg_callback(
      setup_channel_han, target, msg, [=](runtime::BaseMessage* in_msg){
        GetInfoChannel& info = *static_cast<GetInfoChannel*>(in_msg);
        auto const& num_bytes = info.num_bytes;
        create_direct_channel_finish(
          type, han, non_target, action, unique_channel_tag, is_target, num_bytes
        );
      }
    );
    // msg->has_bytes = true;
    // the_msg->send_msg(setup_channel_han, target, msg);
    // create_direct_channel_finish(
    //   type, han, non_target, action, unique_channel_tag, is_target, num_bytes
    // );
  } else {
    return create_direct_channel_finish(
      type, han, non_target, action, channel_tag, is_target, num_bytes
    );
  }
}

byte_t
RDMAManager::lookup_bytes_handler(rdma_handle_t const& han) {
  auto holder_iter = holder.find(han);
  assert(
    holder_iter != holder.end() and "Holder for handler must exist here"
  );
  auto& state = holder_iter->second;
  auto const& num_bytes = state.num_bytes;
  return num_bytes;
}

void
RDMAManager::remove_direct_channel(
  rdma_handle_t const& han, action_t const& action
) {
  auto const& this_node = the_context->get_node();
  auto const target = rdma_handle_manager_t::get_rdma_node(han);

  if (this_node != target) {
    the_msg->send_msg_callback(
      remove_channel_han, target, make_shared_message<DestroyChannel>(
        rdma_type_t::Get, han, no_byte, no_tag
      ),
      [=](runtime::BaseMessage* in_msg){
        rdma_handle_t ch_han = han;
        rdma_handle_manager_t::set_op_type(ch_han, rdma_type_t::Put);
        auto iter = channels.find(ch_han);
        if (iter != channels.end()) {
          iter->second.free_channel();
          channels.erase(iter);
        }
        if (action) {
          action();
        }
      }
    );
  } else {
    rdma_handle_t ch_han = han;
    rdma_handle_manager_t::set_op_type(ch_han, rdma_type_t::Get);
    auto iter = channels.find(ch_han);
    if (iter != channels.end()) {
      iter->second.free_channel();
      channels.erase(iter);
    }
  }

}

tag_t
RDMAManager::next_rdma_channel_tag() {
  tag_t next_tag = next_channel_tag++;
  node_t this_node = the_context->get_node();
  tag_t const& ret = (this_node << 16) | next_tag;
  return ret;
}

/*static*/ void
RDMAManager::register_all_rdma_handlers() {
  the_rdma->get_msg_han =
    CollectiveOps::register_handler([](runtime::BaseMessage* in_msg){
      GetMessage& msg = *static_cast<GetMessage*>(in_msg);
      auto const msg_tag = envelope_get_tag(msg.env);
      auto const op_id = msg.op_id;
      auto const recv_node = msg.requesting;

      auto const& this_node = the_context->get_node();

      debug_print_rdma(
        "%d: get_msg_han: han=%lld, is_user=%s, tag=%d, bytes=%lld\n",
        this_node, msg.rdma_handle, msg.is_user_msg ? "true" : "false",
        msg_tag, msg.num_bytes
      );

      the_rdma->request_get_data(
        &msg, msg.is_user_msg, msg.rdma_handle, msg_tag, msg.num_bytes, nullptr,
        [msg_tag,op_id,recv_node](rdma_get_t data){
          auto const& this_node = the_context->get_node();
          debug_print_rdma("%d: data is ready\n", this_node);
          // @todo send the data here

          // auto const& data_ptr = std::get<0>(data);
          // auto const& num_bytes = std::get<1>(data);

          tag_t recv_tag = no_tag;

          GetBackMessage* new_msg = new GetBackMessage(
            op_id, std::get<1>(data), recv_tag
          );

          auto send_payload = [&](ActiveMessenger::send_fn_t send){
            auto ret = send(data, recv_node, no_tag, [=]{ });
            new_msg->mpi_tag_to_recv = std::get<1>(ret);
          };

          set_put_type(new_msg->env);

          auto deleter = [=]{ delete new_msg; };

          the_msg->send_msg(
            recv_node, the_rdma->get_recv_msg_han, new_msg, send_payload, deleter
          );

          debug_print_rdma("%d: data is sent: recv_tag=%d\n", this_node, recv_tag);
        }
      );
    });

  the_rdma->get_recv_msg_han =
    CollectiveOps::register_handler([](runtime::BaseMessage* in_msg){
      GetBackMessage& msg = *static_cast<GetBackMessage*>(in_msg);
      auto const msg_tag = envelope_get_tag(msg.env);
      auto const op_id = msg.op_id;

      auto direct = the_rdma->try_get_data_ptr_direct(op_id);
      auto get_ptr = std::get<0>(direct);
      auto get_ptr_action = std::get<1>(direct);

      auto const& this_node = the_context->get_node();
      debug_print_rdma(
        "%d: get_recv_msg_han: op=%lld, tag=%d, bytes=%lld, get_ptr=%p, mpi_tag=%d\n",
        this_node, msg.op_id, msg_tag, msg.num_bytes, get_ptr, msg.mpi_tag_to_recv
      );

      if (get_ptr == nullptr) {
        the_msg->recv_data_msg(
          msg.mpi_tag_to_recv, [=](rdma_get_t ptr, action_t deleter){
            the_rdma->trigger_get_recv_data(
              msg.op_id, msg_tag, std::get<0>(ptr), std::get<1>(ptr), deleter
            );
          });
      } else {
        the_msg->recv_data_msg_buffer(
          get_ptr, msg.mpi_tag_to_recv, true, [this_node,get_ptr_action]{
            debug_print_rdma(
              "%d: recv_data_msg_buffer finished\n",
              this_node
            );
            if (get_ptr_action) {
            get_ptr_action();
            }
          }
        );
      }
    });

  the_rdma->put_back_msg_han =
    CollectiveOps::register_handler([](runtime::BaseMessage* in_msg){
      PutBackMessage& msg = *static_cast<PutBackMessage*>(in_msg);
      auto const msg_tag = envelope_get_tag(msg.env);
      auto const op_id = msg.op_id;

      auto const& this_node = the_context->get_node();

      debug_print_rdma(
        "%d: put_back_msg_han: op=%lld\n", this_node, msg.op_id
      );

      the_rdma->trigger_put_back_data(op_id);
    });

  the_rdma->put_recv_msg_han =
    CollectiveOps::register_handler([](runtime::BaseMessage* in_msg){
      PutMessage& msg = *static_cast<PutMessage*>(in_msg);
      auto const msg_tag = envelope_get_tag(msg.env);
      auto const op_id = msg.op_id;
      auto const send_back = msg.send_back;
      auto const recv_tag = msg.mpi_tag_to_recv;

      auto const& this_node = the_context->get_node();

      debug_print_rdma(
        "%d: put_recv_msg_han: op=%lld, tag=%d, bytes=%lld, recv_tag=%d\n",
        this_node, msg.op_id, msg_tag, msg.num_bytes, msg.mpi_tag_to_recv
      );

      assert(
        recv_tag != no_tag and "PutMessage must have recv tag"
      );

      // try to get early access to the ptr for a direct put into user buffer
      auto const& put_ptr = the_rdma->try_put_ptr(msg.rdma_handle, msg_tag);

      debug_print_rdma(
        "%d: put_recv_msg_han: bytes=%lld, recv_tag=%d, put_ptr=%p\n",
        this_node, msg.num_bytes, msg.mpi_tag_to_recv, put_ptr
      );

      if (put_ptr == nullptr) {
        the_msg->recv_data_msg(
          recv_tag, [=](rdma_get_t ptr, action_t deleter){
            debug_print_rdma(
              "%d: put_data: after recv data trigger\n", this_node
            );
            the_rdma->trigger_put_recv_data(
              msg.rdma_handle, msg_tag, std::get<0>(ptr), std::get<1>(ptr),
              [=](){
                debug_print_rdma(
                  "%d: put_data: after put trigger: send_back=%d\n",
                  this_node, send_back
                );
                if (send_back != uninitialized_destination) {
                  PutBackMessage* new_msg = new PutBackMessage(op_id);
                  the_msg->send_msg(
                    send_back, the_rdma->put_back_msg_han, new_msg, [=]{
                      delete new_msg;
                    }
                  );
                }
                deleter();
              }
            );
          });
      } else {
        // do a direct recv into the user buffer
        the_msg->recv_data_msg_buffer(
          put_ptr, recv_tag, true, []{}, [=](rdma_get_t ptr, action_t deleter){
            debug_print_rdma(
              "%d: put_data: recv_data_msg_buffer DIRECT\n", this_node
            );
            if (send_back) {
              PutBackMessage* new_msg = new PutBackMessage(op_id);
              the_msg->send_msg(
                send_back, the_rdma->put_back_msg_han, new_msg, [=]{
                  delete new_msg;
                }
              );
            }
            deleter();
          }
        );
      }
    });

  the_rdma->setup_channel_han =
    CollectiveOps::register_handler([](runtime::BaseMessage* in_msg){
      CreateChannel& msg = *static_cast<CreateChannel*>(in_msg);
      auto const& this_node = the_context->get_node();

      debug_print_rdma_channel(
        "%d: setup_channel_han: han=%lld, target=%d, non_target=%d, "
        "channel_tag=%d\n",
        this_node, msg.rdma_handle, msg.target, msg.non_target, msg.channel_tag
      );

      auto const& num_bytes = the_rdma->lookup_bytes_handler(msg.rdma_handle);

      if (not msg.has_bytes) {
        the_msg->send_callback(make_shared_message<GetInfoChannel>(num_bytes));
      }

      the_rdma->create_direct_channel_internal(
        msg.type, msg.rdma_handle, msg.non_target, nullptr, msg.channel_tag
      );
    });

  the_rdma->remove_channel_han =
    CollectiveOps::register_handler([](runtime::BaseMessage* in_msg){
      DestroyChannel& msg = *static_cast<DestroyChannel*>(in_msg);
      the_rdma->remove_direct_channel(msg.han);
      the_msg->send_callback(make_shared_message<CallbackMessage>());
    });

  the_rdma->remote_channel_han =
    CollectiveOps::register_handler([](runtime::BaseMessage* in_msg){
      ChannelMessage& msg = *static_cast<ChannelMessage*>(in_msg);
      auto const& this_node = the_context->get_node();
      auto const target = rdma_handle_manager_t::get_rdma_node(msg.han);

      debug_print_rdma_channel(
        "%d: remote_channel_han: target=%d, type=%d, han=%lld, tag=%d, "
        "bytes=%lld\n",
        this_node, target, msg.type, msg.han, msg.channel_tag, msg.num_bytes
      );

      the_rdma->create_direct_channel_internal(
        msg.type, msg.han, target, [=]{
          the_msg->send_callback(make_shared_message<CallbackMessage>());
        },
        msg.channel_tag, msg.num_bytes
      );
    });

}


}} // end namespace runtime::rdma
