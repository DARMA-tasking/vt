
#if ! defined __RUNTIME_TRANSPORT_RDMA__
#define __RUNTIME_TRANSPORT_RDMA__

#include "common.h"
#include "function.h"
#include "rdma_common.h"
#include "rdma_state.h"
#include "rdma_handle.h"
#include "rdma_msg.h"
#include "rdma_pending.h"
#include "rdma_channel.h"
#include "rdma_collective.h"

#include <unordered_map>

namespace runtime { namespace rdma {

struct RDMAManager {
  using rdma_bits_t = Bits;
  using rdma_state_t = State;
  using rdma_type_t = Type;
  using rdma_info_t = Info;
  using rdma_pending_t = Pending;
  using rdma_channel_t = Channel;
  using rdma_container_t = std::unordered_map<rdma_handle_t, rdma_state_t>;
  using rdma_live_channels_t = std::unordered_map<rdma_handle_t, rdma_channel_t>;
  using rdma_op_container_t = std::unordered_map<rdma_op_t, rdma_pending_t>;
  using rdma_handle_manager_t = HandleManager;
  using rdma_get_function_t = rdma_state_t::rdma_get_function_t;
  using rdma_put_function_t = rdma_state_t::rdma_put_function_t;
  using rdma_direct_t = std::tuple<rdma_ptr_t, action_t>;

  template <typename T>
  void
  put_typed_data(
    rdma_handle_t const& rdma_handle, T ptr,
    byte_t const& num_elems, tag_t const& tag, action_t cont = no_action,
    action_t action_after_put = no_action
  ) {
    byte_t const num_bytes = num_elems == no_byte ? no_byte : sizeof(T)*num_elems;
    return put_data(
      rdma_handle, static_cast<rdma_ptr_t>(ptr), num_bytes, tag, cont,
      action_after_put
    );
  }

  template <typename T>
  void
  put_typed_data(
    rdma_handle_t const& han, T ptr, byte_t const& num_elems = no_byte,
    action_t cont = no_action, action_t action_after_put = no_action
  ) {
    return put_typed_data<T>(han, ptr, num_elems, no_tag, cont, action_after_put);
  }

  void
  put_data(
    rdma_handle_t const& rdma_handle, rdma_ptr_t const& ptr,
    byte_t const& num_bytes, action_t cont = no_action,
    action_t action_after_put = no_action
  ) {
    return put_data(rdma_handle, ptr, num_bytes, no_tag, cont, action_after_put);
  }

  void
  put_data(
    rdma_handle_t const& rdma_handle, rdma_ptr_t const& ptr,
    byte_t const& num_bytes, tag_t const& tag, action_t cont = no_action,
    action_t action_after_put = no_action
  );

  void
  get_data_info_buf(
    rdma_handle_t const& rdma_handle, rdma_ptr_t const& ptr,
    byte_t const& num_bytes, tag_t const& tag = no_tag,
    action_t next_action = no_action
  );

  template <typename T>
  void
  get_typed_data_info_buf(
    rdma_handle_t const& rdma_handle, T ptr, byte_t const& num_elems,
    tag_t const& tag = no_tag, action_t next_action = no_action
  ) {
    byte_t const num_bytes = num_elems == no_byte ? no_byte : sizeof(T)*num_elems;
    return get_data_info_buf(
      rdma_handle, static_cast<rdma_ptr_t>(ptr), num_bytes, tag, next_action
    );
  }

  template <typename T>
  void
  get_typed_data_info_buf(
    rdma_handle_t const& rdma_handle, T ptr, byte_t const& num_elems,
    action_t na
  ) {
    return get_typed_data_info_buf<T>(rdma_handle, ptr, num_elems, no_tag, na);
  }

  void
  get_data(
    rdma_handle_t const& rdma_handle, rdma_recv_t cont
  ) {
    return get_data(rdma_handle, no_tag, no_byte, cont);
  }

  void
  get_data(
    rdma_handle_t const& rdma_handle, tag_t const& tag, byte_t const& num_bytes,
    rdma_recv_t cont
  );

  template <typename T>
  rdma_handle_t
  register_new_typed_rdma_handler(T ptr, byte_t const& num_elems) {
    byte_t const num_bytes = sizeof(T)*num_elems;
    debug_print_rdma(
      "%d: register_new_typed_rdma_handler ptr=%p, bytes=%lld\n",
      the_context->get_node(), ptr, num_bytes
    );
    return register_new_rdma_handler(
      true, static_cast<rdma_ptr_t>(ptr), num_bytes
    );
  }

  rdma_handle_t
  register_new_rdma_handler(
    bool const& use_default = false, rdma_ptr_t const& ptr = nullptr,
    byte_t const& num_bytes = no_byte, bool const& is_collective = false
  );

  rdma_handle_t
  collective_register_rdma_handler(
    bool const& use_default, rdma_ptr_t const& ptr, byte_t const& num_bytes
  ) {
    return register_new_rdma_handler(use_default, ptr, num_bytes, true);
  }

  void
  unregister_rdma_handler(
    rdma_handle_t const& handle, rdma_type_t const& type = rdma_type_t::GetOrPut,
    tag_t const& tag = no_tag, bool const& use_default = false
  );

  void
  unregister_rdma_handler(
    rdma_handle_t const& handle, rdma_handler_t const& handler,
    tag_t const& tag = no_tag
  );

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
  create_put_channel(
    rdma_handle_t const& han, action_t const& action = nullptr
  );

  void
  setup_put_channel_with_remote(
    rdma_handle_t const& han, node_t const& dest,
    action_t const& action = nullptr
  );

  void
  setup_get_channel_with_remote(
    rdma_handle_t const& han, node_t const& dest,
    action_t const& action = nullptr
  );

  void
  create_get_channel(
    rdma_handle_t const& han, action_t const& action = nullptr
  );

  void
  remove_direct_channel(
    rdma_handle_t const& han, action_t const& action = nullptr
  );

private:
  void
  setup_channel_with_remote(
    rdma_type_t const& type, rdma_handle_t const& han, node_t const& dest,
    action_t const& action
  );

  void
  send_data_channel(
    rdma_type_t const& type, rdma_handle_t const& han, rdma_ptr_t const& ptr,
    byte_t const& num_bytes, action_t cont, action_t action_after_put
  );

  void
  create_direct_channel(
    rdma_type_t const& type, rdma_handle_t const& han,
    action_t const& action = nullptr
  );

  void
  create_direct_channel_internal(
    rdma_type_t const& type, rdma_handle_t const& han, node_t const& non_target,
    action_t const& action = nullptr, tag_t const& channel_tag = no_tag,
    byte_t const& num_bytes = no_byte
  );

  void
  create_direct_channel_finish(
    rdma_type_t const& type, rdma_handle_t const& han, node_t const& non_target,
    action_t const& action, tag_t const& channel_tag, bool const& is_target,
    byte_t const& num_bytes
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

  void
  request_get_data(
    GetMessage* msg, bool const& is_user_msg,
    rdma_handle_t const& rdma_handle, tag_t const& tag, byte_t const& num_bytes,
    rdma_ptr_t const& ptr = nullptr, rdma_continuation_t cont = no_action,
    action_t next_action = no_action
  );

  void
  trigger_get_recv_data(
    rdma_op_t const& op, tag_t const& tag, rdma_ptr_t ptr,
    byte_t const& num_bytes, action_t const& action = no_action
  );

  void
  trigger_put_recv_data(
    rdma_handle_t const& han, tag_t const& tag, rdma_ptr_t ptr,
    byte_t const& num_bytes, action_t const& action
  );

  rdma_direct_t
  try_get_data_ptr_direct(rdma_op_t const& op);

  rdma_ptr_t
  try_put_ptr(
    rdma_handle_t const& han, tag_t const& tag
  );

  void
  trigger_put_back_data(rdma_op_t const& op);

  tag_t
  next_rdma_channel_tag();

  byte_t
  lookup_bytes_handler(rdma_handle_t const& han);

public:
  rdma_handler_t
  allocate_new_rdma_handler();

  static void
  register_all_rdma_handlers();

  // handlers for general functionality
  handler_t get_msg_han = uninitialized_handler;
  handler_t get_recv_msg_han = uninitialized_handler;
  handler_t put_recv_msg_han = uninitialized_handler;
  handler_t put_back_msg_han = uninitialized_handler;

  // handlers for direct rdma channels
  handler_t setup_channel_han = uninitialized_handler;
  handler_t remove_channel_han = uninitialized_handler;
  handler_t remote_channel_han = uninitialized_handler;

private:
  // next local rdma handler (used by State)
  rdma_handler_t cur_rdma_handler = first_rdma_handler;

  // next local rdma identifier
  rdma_identifier_t cur_ident = first_rdma_identifier;

  // next collective rdma identifier
  rdma_identifier_t cur_collective_ident = first_rdma_identifier;

  // rdma state container
  rdma_container_t holder;

  // rdma unique remote operation identifier
  rdma_op_t cur_op = 0;

  // operations that are pending remote interaction
  rdma_op_container_t pending_ops;

  // Live channels that can be used to hardware-level get/put ops
  rdma_live_channels_t channels;

  tag_t next_channel_tag = first_rdma_channel_tag;
};

}} //end namespace runtime::rdma

namespace runtime {

extern std::unique_ptr<rdma::RDMAManager> the_rdma;

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_RDMA__*/
