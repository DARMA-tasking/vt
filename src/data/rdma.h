
#if ! defined __RUNTIME_TRANSPORT_RDMA__
#define __RUNTIME_TRANSPORT_RDMA__

#include "common.h"
#include "function.h"
#include "rdma_common.h"
#include "rdma_types.h"
#include "rdma_map.h"
#include "rdma_region.h"
#include "rdma_state.h"
#include "rdma_handle.h"
#include "rdma_msg.h"
#include "rdma_pending.h"
#include "rdma_channel_lookup.h"
#include "rdma_channel.h"
#include "rdma_group.h"
#include "rdma_action.h"

#include <unordered_map>

namespace runtime { namespace rdma {

struct RDMAManager {
  using rdma_bits_t = Bits;
  using rdma_state_t = State;
  using rdma_type_t = Type;
  using rdma_info_t = Info;
  using rdma_pending_t = Pending;
  using rdma_channel_t = Channel;
  using rdma_region_t = Region;
  using rdma_map_t = Map;
  using rdma_group_t = Group;
  using rdma_action_t = Action;
  using rdma_channel_lookup_t = ChannelLookup;
  using rdma_endpoint_t = Endpoint;
  using rdma_container_t = std::unordered_map<rdma_handle_t, rdma_state_t>;
  using rdma_live_channels_t = std::unordered_map<rdma_channel_lookup_t, rdma_channel_t>;
  using rdma_op_container_t = std::unordered_map<rdma_op_t, rdma_pending_t>;
  using rdma_get_function_t = rdma_state_t::rdma_get_function_t;
  using rdma_put_function_t = rdma_state_t::rdma_put_function_t;
  using rdma_direct_t = std::tuple<RDMA_PtrType, ActionType>;

  template <typename T>
  void
  put_typed_data(
    rdma_handle_t const& rdma_handle, T ptr,
    ByteType const& num_elems, ByteType const& offset, TagType const& tag,
    ActionType cont = no_action, ActionType action_after_put = no_action
  ) {
    ByteType const num_bytes = num_elems == no_byte ? no_byte : sizeof(T)*num_elems;
    ByteType const byte_offset = offset == no_byte ? 0 : sizeof(T)*offset;
    return put_data(
      rdma_handle, static_cast<RDMA_PtrType>(ptr), num_bytes, byte_offset, tag, cont,
      action_after_put
    );
  }

  template <typename T>
  void
  put_typed_data(
    rdma_handle_t const& han, T ptr, ByteType const& num_elems = no_byte,
    ByteType const& offset = no_byte, ActionType cont = no_action,
    ActionType action_after_put = no_action
  ) {
    return put_typed_data<T>(
      han, ptr, num_elems, offset, no_tag, cont, action_after_put
    );
  }

  void
  put_data(
    rdma_handle_t const& rdma_handle, RDMA_PtrType const& ptr,
    ByteType const& num_bytes, ActionType cont = no_action,
    ActionType action_after_put = no_action
  ) {
    return put_data(
      rdma_handle, ptr, num_bytes, no_byte, no_tag, cont, action_after_put
    );
  }

  void
  put_data(
    rdma_handle_t const& rdma_handle, RDMA_PtrType const& ptr,
    ByteType const& num_bytes, ByteType const& offset, TagType const& tag,
    ActionType cont = no_action, ActionType action_after_put = no_action,
    NodeType const& collective_node = uninitialized_destination
  );

  void
  get_data_into_buf(
    rdma_handle_t const& rdma_handle, RDMA_PtrType const& ptr,
    ByteType const& num_bytes, ByteType const& offset,
    TagType const& tag = no_tag, ActionType next_action = no_action,
    ByteType const& elm_size = rdma_default_byte_size,
    NodeType const& collective_node = uninitialized_destination
  );

  void
  get_data_into_buf_collective(
    rdma_handle_t const& rdma_handle, RDMA_PtrType const& ptr,
    ByteType const& num_bytes, ByteType const& elm_size, ByteType const& offset,
    ActionType next_action = no_action
  );

  void
  get_region_typeless(
    rdma_handle_t const& rdma_handle, RDMA_PtrType const& ptr,
    rdma_region_t const& region, ActionType next_action
  );

  template <typename T>
  void
  get_region(
    rdma_handle_t const& rdma_handle, T ptr, rdma_region_t const& region,
    ActionType next_action = no_action
  ) {
    rdma_region_t new_region{region};
    if (not new_region.has_elm_size()) {
      new_region.set_elm_size(sizeof(T));
    }
    return get_region_typeless(rdma_handle, ptr, new_region, next_action);
  }

  template <typename T>
  void
  get_typed_data_info_buf(
    rdma_handle_t const& rdma_handle, T ptr, ByteType const& num_elems,
    ByteType const& elm_offset = no_byte, TagType const& tag = no_tag,
    ActionType next_action = no_action
  ) {
    ByteType const num_bytes = num_elems == no_byte ? no_byte : sizeof(T)*num_elems;
    ByteType const byte_offset = elm_offset == no_byte ? 0 : sizeof(T)*elm_offset;

    return get_data_into_buf(
      rdma_handle, static_cast<RDMA_PtrType>(ptr), num_bytes, byte_offset, tag,
      next_action, sizeof(T)
    );
  }

  template <typename T>
  void
  get_typed_data_info_buf(
    rdma_handle_t const& rdma_handle, T ptr, ByteType const& num_elems,
    ActionType na
  ) {
    return get_typed_data_info_buf<T>(
      rdma_handle, ptr, num_elems, no_byte, no_tag, na
    );
  }

  void
  get_data(
    rdma_handle_t const& rdma_handle, rdma_recv_t cont
  ) {
    return get_data(rdma_handle, no_tag, no_byte, no_byte, cont);
  }

  void
  get_data(
    rdma_handle_t const& rdma_handle, TagType const& tag, ByteType const& num_bytes,
    ByteType const& offset, rdma_recv_t cont
  );

  template <typename T>
  rdma_handle_t
  register_new_typed_rdma_handler(T ptr, ByteType const& num_elems) {
    ByteType const num_bytes = sizeof(T)*num_elems;
    debug_print(
      rdma, node,
      "register_new_typed_rdma_handler ptr=%p, bytes=%lld\n", ptr, num_bytes
    );
    return register_new_rdma_handler(
      true, static_cast<RDMA_PtrType>(ptr), num_bytes
    );
  }

  rdma_handle_t
  register_new_rdma_handler(
    bool const& use_default = false, RDMA_PtrType const& ptr = nullptr,
    ByteType const& num_bytes = no_byte, bool const& is_collective = false
  );

  rdma_handle_t
  collective_register_rdma_handler(
    bool const& use_default, RDMA_PtrType const& ptr, ByteType const& num_bytes
  ) {
    return register_new_rdma_handler(use_default, ptr, num_bytes, true);
  }

  template <typename T>
  rdma_handle_t
  register_collective_typed(
    T ptr, ByteType const& num_total_elems,
    ByteType const& num_elems, rdma_map_t const& map = default_map
  ) {
    ByteType const num_bytes = sizeof(T)*num_elems;
    ByteType const num_total_bytes = sizeof(T)*num_total_elems;
    return register_new_collective(
      true, ptr, num_bytes, num_total_bytes, sizeof(T), map
    );
  }

  rdma_handle_t
  register_new_collective(
    bool const& use_default, RDMA_PtrType const& ptr, ByteType const& num_bytes,
    ByteType const& num_total_bytes, ByteType const& elm_size = rdma_default_byte_size,
    rdma_map_t const& map = default_map
  );

  void
  unregister_rdma_handler(
    rdma_handle_t const& handle, rdma_type_t const& type = rdma_type_t::GetOrPut,
    TagType const& tag = no_tag, bool const& use_default = false
  );

  void
  unregister_rdma_handler(
    rdma_handle_t const& handle, rdma_handler_t const& handler,
    TagType const& tag = no_tag
  );

  rdma_handler_t
  associate_get_function(
    rdma_handle_t const& han, rdma_get_function_t const& fn,
    bool const& any_tag = false, TagType const& tag = no_tag
  ) {
    return associate_rdma_function<rdma_type_t::Get>(han, fn, any_tag, tag);
  }

  rdma_handler_t
  associate_put_function(
    rdma_handle_t const& han, rdma_put_function_t const& fn,
    bool const& any_tag = false, TagType const& tag = no_tag
  ) {
    return associate_rdma_function<rdma_type_t::Put>(han, fn, any_tag, tag);
  }

  void
  new_channel(
    rdma_type_t const& type, rdma_handle_t const& han, NodeType const& target,
    NodeType const& non_target, ActionType const& action
  );

  void
  new_get_channel(
    rdma_handle_t const& han, rdma_endpoint_t const& target,
    rdma_endpoint_t const& non_target, ActionType const& action = nullptr
  ) {
    return new_channel(
      rdma_type_t::Get, han, target.get(), non_target.get(), action
    );
  }

  void
  new_get_channel(
    rdma_handle_t const& han, NodeType const& target,
    NodeType const& non_target, ActionType const& action = nullptr
  ) {
    return new_channel(
      rdma_type_t::Get, han, target, non_target, action
    );
  }

  // void
  // new_get_channel(
  //   rdma_handle_t const& han, rdma_endpoint_t const& endpt,
  //   ActionType const& action = nullptr
  // ) {
  //   NodeType const& target =
  //     endpt.target() ? endpt.get() : uninitialized_destination;
  //   NodeType const& non_target =
  //     not endpt.target() ? endpt.get() : uninitialized_destination;
  //   return new_channel(
  //     rdma_type_t::Get, han, target, non_target, action
  //   );
  // }

  // void
  // new_put_channel(
  //   rdma_handle_t const& han, rdma_endpoint_t const& endpt,
  //   ActionType const& action = nullptr
  // ) {
  //   NodeType const& target =
  //     endpt.target() ? endpt.get() : uninitialized_destination;
  //   NodeType const& non_target =
  //     not endpt.target() ? endpt.get() : uninitialized_destination;
  //   return new_channel(
  //     rdma_type_t::Get, han, target, non_target, action
  //   );
  // }

  void
  new_put_channel(
    rdma_handle_t const& han, NodeType const& target,
    NodeType const& non_target, ActionType const& action = nullptr
  ) {
    return new_channel(
      rdma_type_t::Put, han, target, non_target, action
    );
  }

  void
  sync_local_get_channel(
    rdma_handle_t const& han, ActionType const& action
  ) {
    return sync_local_get_channel(han, uninitialized_destination, action);
  }

  void
  sync_local_get_channel(
    rdma_handle_t const& han, NodeType const& in_target,
    ActionType const& action = nullptr
  ) {
    auto const& this_node = the_context->get_node();
    auto const& target = get_target(han, in_target);
    bool const is_local = true;
    assert(
      this_node != target and "Sync get works with non-target"
    );
    return sync_channel(is_local, han, rdma_type_t::Get, target, this_node, action);
  }

  void
  sync_local_put_channel(
    rdma_handle_t const& han, NodeType const& dest, ActionType const& action = nullptr
  ) {
    return sync_local_put_channel(han, dest, uninitialized_destination, action);
  }

  void
  sync_local_put_channel(
    rdma_handle_t const& han, NodeType const& dest,
    NodeType const& in_target, ActionType const& action = nullptr
  ) {
    auto const& target = get_target(han, in_target);
    bool const is_local = true;
    return sync_channel(is_local, han, rdma_type_t::Put, target, dest, action);
  }

  void
  sync_remote_get_channel(
    rdma_handle_t const& han, NodeType const& in_target = uninitialized_destination,
    ActionType const& action = nullptr
  ) {
    auto const& this_node = the_context->get_node();
    auto const& target = get_target(han, in_target);
    bool const is_local = false;
    assert(
      this_node != target and "Sync get works with non-target"
    );
    return sync_channel(is_local, han, rdma_type_t::Get, target, this_node, action);
  }

  void
  sync_remote_put_channel(rdma_handle_t const& han, ActionType const& action) {
    return sync_remote_put_channel(han, uninitialized_destination, action);
  }

  void
  sync_remote_put_channel(
    rdma_handle_t const& han, NodeType const& in_target,
    ActionType const& action = nullptr
  ) {
    auto const& this_node = the_context->get_node();
    auto const& target = get_target(han, in_target);
    bool const is_local = false;
    assert(
      this_node != target and "Sync remote put channel should be other target"
    );
    return sync_channel(is_local, han, rdma_type_t::Put, target, this_node, action);
  }

  void
  remove_direct_channel(
    rdma_handle_t const& han, NodeType const& override_target = uninitialized_destination,
    ActionType const& action = nullptr
  );

private:
  static NodeType
  get_target(
    rdma_handle_t const& han, NodeType const& in_tar = uninitialized_destination
  ) {
    auto const target = in_tar == uninitialized_destination ?
      rdma_handle_manager_t::get_rdma_node(han) : in_tar;
    return target;
  }

  void
  sync_channel(
    bool const& is_local, rdma_handle_t const& han, rdma_type_t const& type,
    NodeType const& target, NodeType const& non_target, ActionType const& action
  );

  void
  setup_channel_with_remote(
    rdma_type_t const& type, rdma_handle_t const& han, NodeType const& dest,
    ActionType const& action,
    NodeType const& override_target = uninitialized_destination
  );

  void
  send_data_channel(
    rdma_type_t const& type, rdma_handle_t const& han, RDMA_PtrType const& ptr,
    ByteType const& num_bytes, ByteType const& offset, NodeType const& target,
    NodeType const& non_target, ActionType cont, ActionType action_after_put
  );

  void
  create_direct_channel(
    rdma_type_t const& type, rdma_handle_t const& han,
    ActionType const& action = nullptr,
    NodeType const& override_target = uninitialized_destination
  );

  void
  create_direct_channel_internal(
    rdma_type_t const& type, rdma_handle_t const& han, NodeType const& non_target,
    ActionType const& action = nullptr,
    NodeType const& override_target = uninitialized_destination,
    TagType const& channel_tag = no_tag, ByteType const& num_bytes = no_byte
  );

  void
  create_direct_channel_finish(
    rdma_type_t const& type, rdma_handle_t const& han, NodeType const& non_target,
    ActionType const& action, TagType const& channel_tag, bool const& is_target,
    ByteType const& num_bytes,
    NodeType const& override_target = uninitialized_destination
  );

  template <RDMAManager::rdma_type_t rdma_type, typename FunctionT>
  rdma_handler_t
  associate_rdma_function(
    rdma_handle_t const& han, FunctionT const& fn, bool const& any_tag,
    TagType const& tag
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

    return state.template set_rdma_fn<rdma_type, FunctionT>(fn, any_tag, tag);
  }

  void
  request_get_data(
    GetMessage* msg, bool const& is_user_msg,
    rdma_handle_t const& rdma_handle, TagType const& tag, ByteType const& num_bytes,
    ByteType const& offset, RDMA_PtrType const& ptr = nullptr,
    rdma_continuation_t cont = no_action, ActionType next_action = no_action
  );

  void
  trigger_get_recv_data(
    rdma_op_t const& op, TagType const& tag, RDMA_PtrType ptr,
    ByteType const& num_bytes, ActionType const& action = no_action
  );

  void
  trigger_put_recv_data(
    rdma_handle_t const& han, TagType const& tag, RDMA_PtrType ptr,
    ByteType const& num_bytes, ByteType const& offset, ActionType const& action
  );

  rdma_direct_t
  try_get_data_ptr_direct(rdma_op_t const& op);

  RDMA_PtrType
  try_put_ptr(
    rdma_handle_t const& han, TagType const& tag
  );

  void
  trigger_put_back_data(rdma_op_t const& op);

  TagType
  next_rdma_channel_tag();

  ByteType
  lookup_bytes_handler(rdma_handle_t const& han);

  rdma_channel_lookup_t
  make_channel_lookup(
    rdma_handle_t const& han, rdma_type_t const& rdma_op_type,
    NodeType const& target, NodeType const& non_target
  );

  rdma_channel_t*
  find_channel(
    rdma_handle_t const& han, rdma_type_t const& rdma_op_type,
    NodeType const& target, NodeType const& non_target,
    bool const& should_insert = false, bool const& must_exist = false
  );

public:
  rdma_handler_t
  allocate_new_rdma_handler();

  // handler functions for managing rdma operations
  static void
  get_msg(GetMessage* msg);

  static void
  get_recv_msg(GetBackMessage* msg);

  static void
  put_back_msg(PutBackMessage* msg);

  static void
  put_recv_msg(PutMessage* msg);

  // handler functions for managing direct rdma channels
  static void
  setup_channel(CreateChannel* msg);

  static void
  remove_channel(DestroyChannel* msg);

  static void
  remote_channel(ChannelMessage* msg);

  static void
  get_info_channel(GetInfoChannel* msg);

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

  TagType next_channel_tag = first_rdma_channel_tag;
};

}} //end namespace runtime::rdma

namespace runtime {

extern std::unique_ptr<rdma::RDMAManager> the_rdma;

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_RDMA__*/
