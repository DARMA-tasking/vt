
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

template <RDMAManager::rdma_type_t rdma_type, typename FunctionT>
rdma_handler_t
RDMAManager::associate_rdma_function(
  rdma_handle_t const& han, FunctionT const& fn, tag_t const& tag
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

  return state.template set_rdma_fn<rdma_type, FunctionT>(fn, tag);
}

rdma_handler_t
RDMAManager::allocate_new_rdma_handler() {
  rdma_handler_t const handler = cur_rdma_handler++;

  return handler;
}

void
RDMAManager::request_get_data(
  GetMessage* msg, bool const& is_user_msg,
  rdma_handle_t const& rdma_handle, rdma_handler_t const& get_fn_handler
) {
  auto const& this_node = the_context->get_node();
  auto const handler_node = rdma_handle_manager_t::get_rdma_node(rdma_handle);

  assert(
    handler_node == this_node and "Handle must be local to this node"
  );

}

/*static*/ void
RDMAManager::register_all_rdma_handlers() {
  the_rdma->get_msg_han =
    CollectiveOps::register_handler([](runtime::BaseMessage* in_msg){
      GetMessage& msg = *static_cast<GetMessage*>(in_msg);
      the_rdma->request_get_data(
        &msg, msg.is_user_msg, msg.rdma_handle, msg.rdma_handler
      );
    });
}


}} // end namespace runtime::rdma
