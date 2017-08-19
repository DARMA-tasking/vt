
#if ! defined __RUNTIME_TRANSPORT_RDMA__
#define __RUNTIME_TRANSPORT_RDMA__

#include "common.h"
#include "function.h"
#include "rdma_common.h"
#include "rdma_state.h"
#include "rdma_handle.h"
#include "rdma_msg.h"

#include <unordered_map>

namespace runtime { namespace rdma {

struct RDMAPending {
  rdma_recv_t cont = nullptr;

  RDMAPending(rdma_recv_t in_cont)
    : cont(in_cont)
  { }
};

struct RDMAManager {
  using rdma_bits_t = RDMABits;
  using rdma_state_t = RDMAState;
  using rdma_type_t = RDMAType;
  using rdma_info_t = RDMAInfo;
  using rdma_pending_t = RDMAPending;
  using rdma_container_t = std::unordered_map<rdma_handle_t, rdma_state_t>;
  using rdma_op_container_t = std::unordered_map<rdma_op_t, rdma_pending_t>;
  using rdma_handle_manager_t = RDMAHandleManager;
  using rdma_get_function_t = rdma_state_t::rdma_get_function_t;
  using rdma_put_function_t = rdma_state_t::rdma_put_function_t;

  void
  get_data(
    rdma_handle_t const& rdma_handle, tag_t const& tag, rdma_recv_t cont
  );

  void
  get_data(
    rdma_handle_t const& rdma_handle, rdma_recv_t cont
  ) {
    return get_data(rdma_handle, no_tag, cont);
  }

  void
  request_get_data(
    GetMessage* msg, bool const& is_user_msg,
    rdma_handle_t const& rdma_handle, tag_t const& tag, byte_t const& num_bytes,
    rdma_continuation_t cont
  );

  rdma_handle_t
  register_new_rdma_handler(
    byte_t const& num_bytes = no_byte, bool const& is_collective = false
  );

  template <RDMAManager::rdma_type_t rdma_type, typename FunctionT>
  rdma_handler_t
  associate_rdma_function(
    rdma_handle_t const& han, FunctionT const& fn, bool const& any_tag,
    tag_t const& tag
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

    return state.template set_rdma_fn<rdma_type, FunctionT>(fn, any_tag, tag);
  }

  rdma_handler_t
  associate_get_function(
    rdma_handle_t const& han, rdma_get_function_t const& fn,
    bool const& any_tag = false, tag_t const& tag = no_tag
  ) {
    return associate_rdma_function<rdma_type_t::Get>(han, fn, any_tag, tag);
  }

  rdma_handler_t
  associate_put_function(
    rdma_handle_t const& han, rdma_put_function_t const& fn,
    bool const& any_tag = false, tag_t const& tag = no_tag
  ) {
    return associate_rdma_function<rdma_type_t::Put>(han, fn, any_tag, tag);
  }

  void
  trigger_get_recv_data(
    rdma_op_t const& op, tag_t const& tag, rdma_ptr_t ptr,
    byte_t const& num_bytes, action_t const& action = nullptr
  );

  rdma_handler_t
  allocate_new_rdma_handler();

  static void
  register_all_rdma_handlers();

  handler_t get_msg_han = uninitialized_handler;
  handler_t get_recv_msg_han = uninitialized_handler;

private:
  rdma_handler_t cur_rdma_handler = first_rdma_handler;

  rdma_identifier_t cur_ident = first_rdma_identifier;

  rdma_identifier_t cur_col_handle = first_rdma_identifier;

  rdma_container_t holder;

  rdma_op_t cur_op = 0;

  rdma_op_container_t pending_ops;
};

}} //end namespace runtime::rdma

namespace runtime {

extern std::unique_ptr<rdma::RDMAManager> the_rdma;

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_RDMA__*/
