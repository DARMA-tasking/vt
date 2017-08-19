
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
  rdma_handle_t const& handler, FunctionT const& fn, tag_t const& tag
) {
  auto const& this_node = the_context->get_node();
  auto const handler_node = rdma_handle_manager_t::get_rdma_node(handler);

  assert(
    handler_node == this_node and "Handler must be local to this node"
  );

  auto holder_iter = holder.find(handler);
  assert(
    holder_iter != holder.end() and "Holder for handler must exist here"
  );

  auto& state = holder_iter->second;

  return state.set_rdma_fn<rdma_type, FunctionT>(fn, tag);
}

rdma_handler_t
RDMAManager::allocate_new_rdma_handler() {
  rdma_handler_t const handler = cur_rdma_handler++;

  return handler;
}

/*static*/ void
RDMAManager::register_all_rdma_handlers() {

}


}} // end namespace runtime::rdma
