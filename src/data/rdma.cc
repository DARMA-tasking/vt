
#include "rdma.h"
#include "transport.h"

namespace runtime { namespace rdma {

/*static*/ void
RDMAManager::get_msg(GetMessage* msg) {
  auto const msg_tag = envelope_get_tag(msg->env);
  auto const op_id = msg->op_id;
  auto const recv_node = msg->requesting;
  auto const handle = msg->rdma_handle;

  auto const& this_node = the_context->get_node();

  debug_print_rdma(
    "get_msg: han=%lld, is_user=%s, tag=%d, bytes=%lld\n",
    msg->rdma_handle, msg->is_user_msg ? "true" : "false",
    msg_tag, msg->num_bytes
  );

  the_rdma->request_get_data(
    msg, msg->is_user_msg, msg->rdma_handle, msg_tag, msg->num_bytes, msg->offset,
    nullptr, [msg_tag,op_id,recv_node,handle](rdma_get_t data){
      auto const& this_node = the_context->get_node();
      debug_print_rdma("data is ready\n");
      // @todo send the data here

      // auto const& data_ptr = std::get<0>(data);
      // auto const& num_bytes = std::get<1>(data);

      GetBackMessage* new_msg = new GetBackMessage(
        op_id, std::get<1>(data), 0, no_tag, handle, this_node
      );

      auto send_payload = [&](ActiveMessenger::send_fn_t send){
        auto ret = send(data, recv_node, no_tag, [=]{ });
        new_msg->mpi_tag_to_recv = std::get<1>(ret);
      };

      set_put_type(new_msg->env);

      auto deleter = [=]{ delete new_msg; };

      the_msg->send_msg<GetBackMessage, get_recv_msg>(
        recv_node, new_msg, send_payload, deleter
      );

      debug_print_rdma("data is sent: recv_tag=%d\n", new_msg->mpi_tag_to_recv);
    }
  );
}

/*static*/ void
RDMAManager::get_recv_msg(GetBackMessage* msg) {
  auto const msg_tag = envelope_get_tag(msg->env);
  auto const op_id = msg->op_id;

  auto direct = the_rdma->try_get_data_ptr_direct(op_id);
  auto get_ptr = std::get<0>(direct);
  auto get_ptr_action = std::get<1>(direct);

  auto const& this_node = the_context->get_node();
  debug_print_rdma(
    "get_recv_msg: op=%lld, tag=%d, bytes=%lld, get_ptr=%p, mpi_tag=%d, send_back=%d\n",
    msg->op_id, msg_tag, msg->num_bytes, get_ptr, msg->mpi_tag_to_recv, msg->send_back
  );

  if (get_ptr == nullptr) {
    the_msg->recv_data_msg(
      msg->mpi_tag_to_recv, msg->send_back, [=](rdma_get_t ptr, action_t deleter){
        the_rdma->trigger_get_recv_data(
          msg->op_id, msg_tag, std::get<0>(ptr), std::get<1>(ptr), deleter
        );
      });
  } else {
    the_msg->recv_data_msg_buffer(
      get_ptr, msg->mpi_tag_to_recv, msg->send_back, true, [this_node,get_ptr_action]{
        debug_print_rdma("recv_data_msg_buffer finished\n");
        if (get_ptr_action) {
          get_ptr_action();
        }
      }
    );
  }
}

/*static*/ void
RDMAManager::put_back_msg(PutBackMessage* msg) {
  auto const msg_tag = envelope_get_tag(msg->env);
  auto const op_id = msg->op_id;

  auto const& this_node = the_context->get_node();

  debug_print_rdma(
    "put_back_msg: op=%lld\n", msg->op_id
  );

  the_rdma->trigger_put_back_data(op_id);
}

/*static*/ void
RDMAManager::put_recv_msg(PutMessage* msg) {
  auto const msg_tag = envelope_get_tag(msg->env);
  auto const op_id = msg->op_id;
  auto const send_back = msg->send_back;
  auto const recv_node = msg->recv_node;
  auto const recv_tag = msg->mpi_tag_to_recv;

  auto const& this_node = the_context->get_node();

  debug_print_rdma(
    "put_recv_msg: op=%lld, tag=%d, bytes=%lld, recv_tag=%d\n",
    msg->op_id, msg_tag, msg->num_bytes, msg->mpi_tag_to_recv
  );

  assert(
    recv_tag != no_tag and "PutMessage must have recv tag"
  );

  // try to get early access to the ptr for a direct put into user buffer
  auto const& put_ptr = the_rdma->try_put_ptr(msg->rdma_handle, msg_tag);

  debug_print_rdma(
    "put_recv_msg: bytes=%lld, recv_tag=%d, put_ptr=%p\n",
    msg->num_bytes, msg->mpi_tag_to_recv, put_ptr
  );

  if (put_ptr == nullptr) {
    the_msg->recv_data_msg(
      recv_tag, recv_node, [=](rdma_get_t ptr, action_t deleter){
        debug_print_rdma("put_data: after recv data trigger\n");
        the_rdma->trigger_put_recv_data(
          msg->rdma_handle, msg_tag, std::get<0>(ptr), std::get<1>(ptr),
          msg->offset, [=](){
            debug_print_rdma(
              "put_data: after put trigger: send_back=%d\n",
              send_back
            );
            if (send_back != uninitialized_destination) {
              PutBackMessage* new_msg = new PutBackMessage(op_id);
              the_msg->send_msg<PutBackMessage, put_back_msg>(
                send_back, new_msg, [=]{ delete new_msg; }
              );
            }
            deleter();
          }
        );
      });
  } else {
    auto const& put_ptr_offset =
      msg->offset != no_byte ? static_cast<char*>(put_ptr) + msg->offset : put_ptr;

    // do a direct recv into the user buffer
    the_msg->recv_data_msg_buffer(
      put_ptr_offset, recv_tag, recv_node, true, []{},
      [=](rdma_get_t ptr, action_t deleter){
        debug_print_rdma(
          "put_data: recv_data_msg_buffer DIRECT: offset=%lld\n",
          msg->offset
        );
        if (send_back) {
          PutBackMessage* new_msg = new PutBackMessage(op_id);
          the_msg->send_msg<PutBackMessage, put_back_msg>(
            send_back, new_msg, [=]{ delete new_msg; }
          );
        }
        deleter();
      }
    );
  }
}

/*static*/ void
RDMAManager::setup_channel(CreateChannel* msg) {
  auto const& this_node = the_context->get_node();

  debug_print_rdma_channel(
    "setup_channel: han=%lld, target=%d, non_target=%d, "
    "channel_tag=%d\n",
    msg->rdma_handle, msg->target, msg->non_target, msg->channel_tag
  );

  auto const& num_bytes = the_rdma->lookup_bytes_handler(msg->rdma_handle);

  if (not msg->has_bytes) {
    the_msg->send_callback(make_shared_message<GetInfoChannel>(num_bytes));
  }

  the_rdma->create_direct_channel_internal(
    msg->type, msg->rdma_handle, msg->non_target, nullptr, msg->channel_tag
  );
}

/*static*/ void
RDMAManager::remove_channel(DestroyChannel* msg) {
  the_rdma->remove_direct_channel(msg->han);
  the_msg->send_callback(make_shared_message<CallbackMessage>());
}

/*static*/ void
RDMAManager::remote_channel(ChannelMessage* msg) {
  auto const& this_node = the_context->get_node();
  auto const target = rdma_handle_manager_t::get_rdma_node(msg->han);

  debug_print_rdma_channel(
    "remote_channel_han: target=%d, type=%d, han=%lld, tag=%d, "
    "bytes=%lld\n",
    target, msg->type, msg->han, msg->channel_tag, msg->num_bytes
  );

  the_rdma->create_direct_channel_internal(
    msg->type, msg->han, target, [=]{
      the_msg->send_callback(make_shared_message<CallbackMessage>());
    },
    msg->channel_tag, msg->num_bytes
  );
}

rdma_handle_t
RDMAManager::register_new_collective(
  bool const& use_default, rdma_ptr_t const& ptr, byte_t const& num_bytes,
  byte_t const& num_total_bytes, byte_t const& elm_size, rdma_map_t const& map
) {
  auto const& han = register_new_rdma_handler(use_default, ptr, num_bytes, true);

  auto iter = holder.find(han);
  assert(
    iter != holder.end() and "Handler must exist"
  );

  auto& state = iter->second;

  state.group_info = std::make_unique<rdma_group_t>(
    map, num_bytes / elm_size, the_context->get_num_nodes(), elm_size
  );

  return han;
}

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
  rdma_handle_manager_t::set_rdma_identifier(new_handle, new_identifier);

  if (not is_collective) {
    rdma_handle_manager_t::set_rdma_node(new_handle, this_node);
  }

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
  GetMessage* msg, bool const& is_user_msg, rdma_handle_t const& han,
  tag_t const& tag, byte_t const& num_bytes, byte_t const& offset,
  rdma_ptr_t const& ptr, rdma_continuation_t cont, action_t next_action
) {
  auto const& this_node = the_context->get_node();
  auto const handler_node = rdma_handle_manager_t::get_rdma_node(han);
  auto const& is_collective = rdma_handle_manager_t::is_collective(han);

  assert(
    (is_collective or handler_node == this_node)
    and "Handle must be local to this node"
  );

  auto holder_iter = holder.find(han);
  assert(
    holder_iter != holder.end() and "Holder for handler must exist here"
  );

  auto& state = holder_iter->second;

  rdma_info_t info(
    rdma_type_t::Get, num_bytes, offset, tag, cont, next_action, ptr
  );

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
  byte_t const& num_bytes, byte_t const& offset, action_t const& action
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

  rdma_info_t info(
    rdma_type_t::Put, num_bytes, offset, tag, nullptr, action, ptr
  );

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
RDMAManager::sync_get_channel(
  bool const& is_local, rdma_handle_t const& han, action_t const& action
) {
  auto const& this_node = the_context->get_node();
  rdma_type_t const& type = rdma_type_t::Get;
  rdma_handle_t ch_han = han;
  rdma_handle_manager_t::set_op_type(ch_han, type);

  debug_print_rdma_channel(
    "sync_get_channel: is_local=%s, han=%lld\n", print_bool(is_local), han
  );

  auto channel_iter = channels.find(ch_han);
  if (channel_iter != channels.end()) {
    Channel& channel = channel_iter->second;
    if (is_local) {
      channel.sync_channel_local();
    } else {
      channel.sync_channel_global();
    }
  }

  if (action) {
    action();
  }
}

void
RDMAManager::sync_put_channel(
  bool const& is_local, rdma_handle_t const& han, action_t const& action
) {
  auto const& this_node = the_context->get_node();
  rdma_type_t const& type = rdma_type_t::Put;
  rdma_handle_t ch_han = han;
  rdma_handle_manager_t::set_op_type(ch_han, type);

  debug_print_rdma_channel(
    "sync_put_channel: is_local=%s, han=%lld\n", print_bool(is_local), han
  );

  auto channel_iter = channels.find(ch_han);
  if (channel_iter != channels.end()) {
    Channel& channel = channel_iter->second;
    if (is_local) {
      channel.sync_channel_local();
    } else {
      channel.sync_channel_global();
    }
  }

  if (action) {
    action();
  }
}

void
RDMAManager::send_data_channel(
  rdma_type_t const& type, rdma_handle_t const& han, rdma_ptr_t const& ptr,
  byte_t const& num_bytes, byte_t const& offset, action_t cont,
  action_t action_after_remote_op
) {
  rdma_handle_t ch_han = han;
  rdma_handle_manager_t::set_op_type(ch_han, type);

  auto channel_iter = channels.find(ch_han);
  assert(
    channel_iter != channels.end() and "Channel must exist"
  );
  Channel& channel = channel_iter->second;

  channel.write_data_to_channel(ptr, num_bytes, offset);

  if (cont) {
    cont();
  }

  if (type == rdma_type_t::Put and action_after_remote_op) {
    sync_remote_put_channel(han, [=]{
      action_after_remote_op();
    });
  } else if (type == rdma_type_t::Get and action_after_remote_op) {
    sync_remote_get_channel(han, [=]{
      action_after_remote_op();
    });
  }
}

void
RDMAManager::put_data(
  rdma_handle_t const& han, rdma_ptr_t const& ptr, byte_t const& num_bytes,
  byte_t const& offset, tag_t const& tag, action_t cont,
  action_t action_after_put
) {
  auto const& this_node = the_context->get_node();
  auto const put_node = rdma_handle_manager_t::get_rdma_node(han);
  auto const& is_collective = rdma_handle_manager_t::is_collective(han);

  if (put_node != this_node) {
    rdma_handle_t ch_han = han;
    rdma_handle_manager_t::set_op_type(ch_han, rdma_type_t::Put);

    bool const send_via_channel =
      channels.find(ch_han) != channels.end() and tag == no_tag and
      not is_collective;

    if (send_via_channel) {
      return send_data_channel(
        rdma_type_t::Put, han, ptr, num_bytes, offset, cont, action_after_put
      );
    } else {
      rdma_op_t const new_op = cur_op++;

      tag_t recv_tag = no_tag;

      PutMessage* msg = new PutMessage(
        new_op, num_bytes, offset, no_tag, han,
        action_after_put ? this_node : uninitialized_destination,
        this_node
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
        "put_data: sending: ptr=%p, num_bytes=%lld, recv_tag=%d, offset=%lld\n",
        ptr, num_bytes, recv_tag, offset
      );

      set_put_type(msg->env);

      if (tag != no_tag) {
        envelope_set_tag(msg->env, tag);
      }

      auto deleter = [=]{ delete msg; };

      the_msg->send_msg<PutMessage, put_recv_msg>(
        put_node, msg, send_payload, deleter
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
      han, tag, ptr, num_bytes, offset, [=](){
        debug_print_rdma("put_data: local data is put\n");
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
RDMAManager::get_region_typeless(
  rdma_handle_t const& han, rdma_ptr_t const& ptr, rdma_region_t const& region,
  action_t next_action
) {
  auto const& this_node = the_context->get_node();
  auto const& is_collective = rdma_handle_manager_t::is_collective(han);

  if (is_collective) {
    printf(
      "%d: get_region_typeless: han=%lld, ptr=%p, region=%s\n",
      this_node,han,ptr,region.region_to_buf().c_str()
    );

    auto holder_iter = holder.find(han);
    assert(
      holder_iter != holder.end() and "Holder for handler must exist here"
    );

    auto& state = holder_iter->second;

    assert(state.group_info != nullptr);

    auto group = state.group_info.get();

    auto action = new Action(1, next_action);

    group->walk_region(region, [&](
      node_t node, rdma_block_elm_range_t rng, rdma_elm_t lo, rdma_elm_t hi
    ) {
      auto const& blk = std::get<0>(rng);
      auto const& blk_lo = std::get<1>(rng);
      auto const& blk_hi = std::get<2>(rng);
      auto const& elm_size = region.elm_size;
      auto const& rlo = region.lo;
      auto const& rhi = region.hi;
      auto const& roffset = lo - rlo;
      auto const& ptr_offset = static_cast<char*>(ptr) + (roffset * elm_size);
      auto const& block_offset = (lo - blk_lo) * elm_size;

      printf(
        "\t: node=%d, lo=%lld, hi=%lld, blk=%lld, blk_lo=%lld, blk_hi=%lld, "
        "block_offset=%lld, ptr_offset={%lld,%lld}\n",
        node, lo, hi, blk, blk_lo, blk_hi, block_offset, roffset, roffset*elm_size
      );

      action->add_dep();

      get_data_into_buf(
        han, ptr_offset, (hi-lo)*elm_size, block_offset, no_tag, [=]{
          action->release();
        }, elm_size, node
      );
    });

    action->release();
  } else {
    assert(
      is_collective and "Getting regions only works with collective handles"
    );
  }
}


void
RDMAManager::get_data_into_buf_collective(
  rdma_handle_t const& han, rdma_ptr_t const& ptr, byte_t const& num_bytes,
  byte_t const& elm_size, byte_t const& offset, action_t next_action
) {
  auto const& this_node = the_context->get_node();

  printf(
    "%d: get_data_into_buf_collective: han=%lld, ptr=%p, bytes=%lld, offset=%lld\n",
    this_node,han,ptr,num_bytes,offset
  );

  auto const& num_elems = num_bytes / (elm_size / rdma_default_byte_size);

  auto const& a_offset = offset == no_offset ? 0 : offset;
  rdma_region_t const region(
    a_offset / elm_size, (a_offset + num_bytes)/elm_size, 1, elm_size
  );

  return get_region_typeless(han, ptr, region, next_action);
}

void
RDMAManager::get_data_into_buf(
  rdma_handle_t const& han, rdma_ptr_t const& ptr, byte_t const& num_bytes,
  byte_t const& offset, tag_t const& tag, action_t next_action,
  byte_t const& elm_size, node_t const& collective_node
) {
  auto const& this_node = the_context->get_node();
  auto const& handle_get_node = rdma_handle_manager_t::get_rdma_node(han);
  auto const& is_collective = rdma_handle_manager_t::is_collective(han);

  printf(
    "%d: get_data_into_buf: han=%lld, is_collective=%s, get_node=%d, "
    "elm_size=%lld, num_bytes=%lld, offset=%lld\n",
    this_node,han,print_bool(is_collective),handle_get_node,elm_size,num_bytes,offset
  );

  auto const& get_node = is_collective ? collective_node : handle_get_node;

  if (is_collective and collective_node == uninitialized_destination) {
    return get_data_into_buf_collective(
      han, ptr, num_bytes, elm_size, offset, next_action
    );
  } else {
    // non-collective get
    if (get_node != this_node) {
      rdma_handle_t ch_han = han;
      rdma_handle_manager_t::set_op_type(ch_han, rdma_type_t::Get);

      bool const send_via_channel =
        channels.find(ch_han) != channels.end() and tag == no_tag and
        not is_collective;

      if (send_via_channel) {
        return send_data_channel(
          rdma_type_t::Get, han, ptr, num_bytes, offset, nullptr, next_action
        );
      } else {
        rdma_op_t const new_op = cur_op++;

        GetMessage* msg = new GetMessage(new_op, this_node, han, num_bytes, offset);
        if (tag != no_tag) {
          envelope_set_tag(msg->env, tag);
        }
        the_msg->send_msg<GetMessage, get_msg>(get_node, msg, [=]{ delete msg; });

        pending_ops.emplace(
          std::piecewise_construct,
          std::forward_as_tuple(new_op),
          std::forward_as_tuple(rdma_pending_t{ptr, next_action})
        );
      }
    } else {
      debug_print_rdma(
        "get_data: local direct into buf, ptr=%p\n", ptr
      );
      the_rdma->request_get_data(
        nullptr, false, han, tag, num_bytes, offset, ptr, nullptr, next_action
      );
    }
  }
}

void
RDMAManager::get_data(
  rdma_handle_t const& han, tag_t const& tag, byte_t const& num_bytes,
  byte_t const& offset, rdma_recv_t cont
) {
  auto const& this_node = the_context->get_node();
  auto const get_node = rdma_handle_manager_t::get_rdma_node(han);

  if (get_node != this_node) {
    rdma_op_t const new_op = cur_op++;

    GetMessage* msg = new GetMessage(new_op, this_node, han, num_bytes, offset);
    if (tag != no_tag) {
      envelope_set_tag(msg->env, tag);
    }
    the_msg->send_msg<GetMessage, get_msg>(get_node, msg, [=]{ delete msg; });

    pending_ops.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(new_op),
      std::forward_as_tuple(rdma_pending_t{cont})
    );
  } else {
    the_rdma->request_get_data(
      nullptr, false, han, tag, num_bytes, offset, nullptr, [cont](rdma_get_t data){
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

    the_msg->send_msg_callback<ChannelMessage, remote_channel>(
      other_node, msg, [=](runtime::BaseMessage*){
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
      "create_direct_channel: han=%lld, is_target=%s, state ptr=%p, bytes=%lld\n",
      han, print_bool(is_target), target_ptr, target_num_bytes
    );
  }

  rdma_handle_t ch_han = han;
  rdma_handle_manager_t::set_op_type(ch_han, type);

  auto iter = channels.find(ch_han);
  if (iter == channels.end()) {
    debug_print_rdma_channel(
      "create_direct_channel: han=%lld, is_target=%s, creating\n",
      han, print_bool(is_target)
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
      "create_direct_channel: han=%lld, target=%d, already created!\n",
      han, target
    );
    if (action) {
      action();
    }
    return;
  }

  debug_print_rdma_channel(
    "create_direct_channel: han=%lld, target=%d, op_type=%d, is_target=%s, "
    "channel_tag=%d\n",
    han, target, rdma_op_type, print_bool(is_target), channel_tag
  );

  if (not is_target and channel_tag == no_tag and num_bytes == no_byte) {
    assert(
      channel_tag == no_tag and "Should not have tag assigned"
    );

    auto const& unique_channel_tag = next_rdma_channel_tag();

    debug_print_rdma_channel(
      "create_direct_channel: generate unique tag: channel_tag=%d\n",
      unique_channel_tag
    );

    assert(
      channel_tag == no_tag and
      "Should not have a tag assigned at this point"
    );

    auto msg = make_shared_message<CreateChannel>(
      type, han, unique_channel_tag, target, this_node
    );

    the_msg->send_msg_callback<CreateChannel, setup_channel>(
      target, msg, [=](runtime::BaseMessage* in_msg){
        GetInfoChannel& info = *static_cast<GetInfoChannel*>(in_msg);
        auto const& num_bytes = info.num_bytes;
        create_direct_channel_finish(
          type, han, non_target, action, unique_channel_tag, is_target, num_bytes
        );
      }
    );
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
    DestroyChannel* msg = make_shared_message<DestroyChannel>(
      rdma_type_t::Get, han, no_byte, no_tag
    );
    the_msg->send_msg_callback<DestroyChannel, remove_channel>(
      target, msg, [=](runtime::BaseMessage* in_msg){
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

}} // end namespace runtime::rdma
