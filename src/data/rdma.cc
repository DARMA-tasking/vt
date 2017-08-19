
#include "rdma.h"
#include "transport.h"

namespace runtime { namespace rdma {

rdma_handle_t
RDMAManager::register_new_rdma_handler(
  byte_t const& num_bytes, bool const& is_collective
) {
  auto const& this_node = the_context->get_node();

  rdma_handler_t new_handle = 0;
  rdma_identifier_t const& new_identifier = cur_ident++;

  rdma_handle_manager_t::set_is_collective(new_handle, is_collective);
  rdma_handle_manager_t::set_is_sized(new_handle, is_collective);
  rdma_handle_manager_t::set_rdma_node(new_handle, this_node);
  rdma_handle_manager_t::set_rdma_identifier(new_handle, new_identifier);

  holder.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(new_handle),
    std::forward_as_tuple(rdma_state_t{new_handle, num_bytes})
  );

  return new_handle;
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
RDMAManager::get_data(
  rdma_handle_t const& han, tag_t const& tag, rdma_recv_t cont
) {
  auto const& this_node = the_context->get_node();
  auto const get_node = rdma_handle_manager_t::get_rdma_node(han);

  if (get_node != this_node) {
    rdma_op_t const new_op = cur_op++;

    GetMessage* msg = new GetMessage(new_op, this_node, han);
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
      nullptr, false, han, tag, no_byte, [cont](rdma_get_t data){
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
}


}} // end namespace runtime::rdma
