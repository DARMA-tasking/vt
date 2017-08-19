
#include "rdma.h"
#include "transport.h"

namespace runtime { namespace rdma {

rdma_handle_t
RDMAManager::register_new_rdma_handler(
  bool const& use_default, rdma_ptr_t const& ptr, byte_t const& num_bytes
) {
  auto const& this_node = the_context->get_node();

  rdma_handler_t new_handle = 0;
  rdma_identifier_t const& new_identifier = cur_ident++;

  bool const is_collective = false;
  bool const is_sized = false;

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
  rdma_continuation_t cont
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

  rdma_info_t info(rdma_type_t::Get, num_bytes, tag, cont);

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

  iter->second.cont(ptr, num_bytes);

  pending_ops.erase(iter);

  if (action != nullptr) {
    action();
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
RDMAManager::put_data(
  rdma_handle_t const& han, rdma_ptr_t const& ptr, byte_t const& num_bytes,
  tag_t const& tag, action_t cont, action_t action_after_put
) {
  auto const& this_node = the_context->get_node();
  auto const put_node = rdma_handle_manager_t::get_rdma_node(han);

  if (put_node != this_node) {
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

    printf(
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
  } else {
    the_rdma->trigger_put_recv_data(
      han, tag, ptr, num_bytes, [=](){
        printf("%d: put_data: local data is put\n", this_node);
        printf("local: data is ready: put\n");
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
      nullptr, false, han, tag, num_bytes, [cont](rdma_get_t data){
        printf("local: data is ready\n");
        cont(std::get<0>(data), std::get<1>(data));
      }
    );
  }
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
      printf(
        "%d: get_msg_han: han=%lld, is_user=%s, tag=%d, bytes=%lld\n",
        this_node, msg.rdma_handle, msg.is_user_msg ? "true" : "false",
        msg_tag, msg.num_bytes
      );

      the_rdma->request_get_data(
        &msg, msg.is_user_msg, msg.rdma_handle, msg_tag, msg.num_bytes,
        [msg_tag,op_id,recv_node](rdma_get_t data){
          auto const& this_node = the_context->get_node();
          printf("%d: data is ready\n", this_node);
          // @todo send the data here

          // auto const& data_ptr = std::get<0>(data);
          // auto const& num_bytes = std::get<1>(data);

          tag_t recv_tag = no_tag;

          auto send_payload = [&](ActiveMessenger::send_fn_t send){
            auto ret = send(data, recv_node, no_tag, [=]{ });
            recv_tag = std::get<1>(ret);
          };

          GetBackMessage* new_msg = new GetBackMessage(
            op_id, std::get<1>(data), recv_tag
          );

          set_put_type(new_msg->env);

          auto deleter = [=]{ delete new_msg; };

          the_msg->send_msg(
            recv_node, the_rdma->get_recv_msg_han, new_msg, send_payload, deleter
          );
        }
      );
    });

  the_rdma->get_recv_msg_han =
    CollectiveOps::register_handler([](runtime::BaseMessage* in_msg){
      GetBackMessage& msg = *static_cast<GetBackMessage*>(in_msg);
      auto const msg_tag = envelope_get_tag(msg.env);
      auto const op_id = msg.op_id;

      auto const& this_node = the_context->get_node();
      printf(
        "%d: get_recv_msg_han: op=%lld, tag=%d, bytes=%lld\n",
        this_node, msg.op_id, msg_tag, msg.num_bytes
      );

      the_msg->recv_data_msg(
        msg.mpi_tag_to_recv, [=](rdma_get_t ptr, action_t deleter){
        the_rdma->trigger_get_recv_data(
          msg.op_id, msg_tag, std::get<0>(ptr), std::get<1>(ptr), deleter
        );
      });
    });

  the_rdma->put_back_msg_han =
    CollectiveOps::register_handler([](runtime::BaseMessage* in_msg){
      PutBackMessage& msg = *static_cast<PutBackMessage*>(in_msg);
      auto const msg_tag = envelope_get_tag(msg.env);
      auto const op_id = msg.op_id;

      auto const& this_node = the_context->get_node();

      printf(
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

      printf(
        "%d: put_recv_msg_han: op=%lld, tag=%d, bytes=%lld, recv_tag=%d\n",
        this_node, msg.op_id, msg_tag, msg.num_bytes, msg.mpi_tag_to_recv
      );

      assert(
        recv_tag != no_tag and "PutMessage must have recv tag"
      );

      // try to get early access to the ptr for a direct put into user buffer
      auto const& put_ptr = the_rdma->try_put_ptr(msg.rdma_handle, msg_tag);

      if (put_ptr == nullptr) {
        the_msg->recv_data_msg(
          recv_tag, [=](rdma_get_t ptr, action_t deleter){
            printf("%d: put_data: after recv data trigger\n", this_node);
            the_rdma->trigger_put_recv_data(
              msg.rdma_handle, msg_tag, std::get<0>(ptr), std::get<1>(ptr),
              [=](){
                printf("%d: put_data: after put trigger\n", this_node);
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
          });
      } else {
        // do a direct recv into the user buffer
        the_msg->recv_data_msg_buffer(
          put_ptr, recv_tag, true, []{}, [=](rdma_get_t ptr, action_t deleter){
            printf("%d: put_data: after recv put data trigger\n", this_node);
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
}


}} // end namespace runtime::rdma
