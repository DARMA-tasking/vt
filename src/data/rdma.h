
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

namespace vt { namespace rdma {

struct RDMAManager {
  using RDMA_BitsType = Bits;
  using RDMA_StateType = State;
  using RDMA_TypeType = Type;
  using RDMA_InfoType = Info;
  using RDMA_PendingType = Pending;
  using RDMA_ChannelType = Channel;
  using RDMA_RegionType = Region;
  using RDMA_MapType = Map;
  using RDMA_GroupType = Group;
  using RDMA_ActionType = Action;
  using RDMA_ChannelLookupType = ChannelLookup;
  using RDMA_EndpointType = Endpoint;
  using RDMA_ContainerType = std::unordered_map<RDMA_HandleType, RDMA_StateType>;
  using RDMA_LiveChannelsType = std::unordered_map<RDMA_ChannelLookupType, RDMA_ChannelType>;
  using RDMA_OpContainerType = std::unordered_map<RDMA_OpType, RDMA_PendingType>;
  using RDMA_GetFunctionType = RDMA_StateType::RDMA_GetFunctionType;
  using RDMA_PutFunctionType = RDMA_StateType::RDMA_PutFunctionType;
  using RDMA_DirectType = std::tuple<RDMA_PtrType, ActionType>;

  template <typename T>
  void put_typed_data(
    RDMA_HandleType const& rdma_handle, T ptr,
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
  void put_typed_data(
    RDMA_HandleType const& han, T ptr, ByteType const& num_elems = no_byte,
    ByteType const& offset = no_byte, ActionType cont = no_action,
    ActionType action_after_put = no_action
  ) {
    return put_typed_data<T>(
      han, ptr, num_elems, offset, no_tag, cont, action_after_put
    );
  }

  void put_data(
    RDMA_HandleType const& rdma_handle, RDMA_PtrType const& ptr,
    ByteType const& num_bytes, ActionType cont = no_action,
    ActionType action_after_put = no_action
  ) {
    return put_data(
      rdma_handle, ptr, num_bytes, no_byte, no_tag, cont, action_after_put
    );
  }

  void put_data(
    RDMA_HandleType const& rdma_handle, RDMA_PtrType const& ptr,
    ByteType const& num_bytes, ByteType const& offset, TagType const& tag,
    ActionType cont = no_action, ActionType action_after_put = no_action,
    NodeType const& collective_node = uninitialized_destination
  );

  void  get_data_into_buf(
    RDMA_HandleType const& rdma_handle, RDMA_PtrType const& ptr,
    ByteType const& num_bytes, ByteType const& offset,
    TagType const& tag = no_tag, ActionType next_action = no_action,
    ByteType const& elm_size = rdma_default_byte_size,
    NodeType const& collective_node = uninitialized_destination
  );

  void get_data_into_buf_collective(
    RDMA_HandleType const& rdma_handle, RDMA_PtrType const& ptr,
    ByteType const& num_bytes, ByteType const& elm_size, ByteType const& offset,
    ActionType next_action = no_action
  );

  void get_region_typeless(
    RDMA_HandleType const& rdma_handle, RDMA_PtrType const& ptr,
    RDMA_RegionType const& region, ActionType next_action
  );

  template <typename T>
  void get_region(
    RDMA_HandleType const& rdma_handle, T ptr, RDMA_RegionType const& region,
    ActionType next_action = no_action
  ) {
    RDMA_RegionType new_region{region};
    if (not new_region.has_elm_size()) {
      new_region.set_elm_size(sizeof(T));
    }
    return get_region_typeless(rdma_handle, ptr, new_region, next_action);
  }

  template <typename T>
  void get_typed_data_info_buf(
    RDMA_HandleType const& rdma_handle, T ptr, ByteType const& num_elems,
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
  void get_typed_data_info_buf(
    RDMA_HandleType const& rdma_handle, T ptr, ByteType const& num_elems,
    ActionType na
  ) {
    return get_typed_data_info_buf<T>(
      rdma_handle, ptr, num_elems, no_byte, no_tag, na
    );
  }

  void get_data(RDMA_HandleType const& rdma_handle, RDMA_RecvType cont) {
    return get_data(rdma_handle, no_tag, no_byte, no_byte, cont);
  }

  void get_data(
    RDMA_HandleType const& rdma_handle, TagType const& tag, ByteType const& num_bytes,
    ByteType const& offset, RDMA_RecvType cont
  );

  template <typename T>
  RDMA_HandleType register_new_typed_rdma_handler(T ptr, ByteType const& num_elems) {
    ByteType const num_bytes = sizeof(T)*num_elems;
    debug_print(
      rdma, node,
      "register_new_typed_rdma_handler ptr=%p, bytes=%lld\n", ptr, num_bytes
    );
    return register_new_rdma_handler(
      true, static_cast<RDMA_PtrType>(ptr), num_bytes
    );
  }

  RDMA_HandleType register_new_rdma_handler(
    bool const& use_default = false, RDMA_PtrType const& ptr = nullptr,
    ByteType const& num_bytes = no_byte, bool const& is_collective = false
  );

  RDMA_HandleType collective_register_rdma_handler(
    bool const& use_default, RDMA_PtrType const& ptr, ByteType const& num_bytes
  ) {
    return register_new_rdma_handler(use_default, ptr, num_bytes, true);
  }

  template <typename T>
  RDMA_HandleType register_collective_typed(
    T ptr, ByteType const& num_total_elems, ByteType const& num_elems,
    RDMA_MapType const& map = default_map
  ) {
    ByteType const num_bytes = sizeof(T)*num_elems;
    ByteType const num_total_bytes = sizeof(T)*num_total_elems;
    return register_new_collective(
      true, ptr, num_bytes, num_total_bytes, sizeof(T), map
    );
  }

  RDMA_HandleType register_new_collective(
    bool const& use_default, RDMA_PtrType const& ptr, ByteType const& num_bytes,
    ByteType const& num_total_bytes, ByteType const& elm_size = rdma_default_byte_size,
    RDMA_MapType const& map = default_map
  );

  void unregister_rdma_handler(
    RDMA_HandleType const& handle, RDMA_TypeType const& type = RDMA_TypeType::GetOrPut,
    TagType const& tag = no_tag, bool const& use_default = false
  );

  void unregister_rdma_handler(
    RDMA_HandleType const& handle, RDMA_HandlerType const& handler,
    TagType const& tag = no_tag
  );

  RDMA_HandlerType associate_get_function(
    RDMA_HandleType const& han, RDMA_GetFunctionType const& fn,
    bool const& any_tag = false, TagType const& tag = no_tag
  ) {
    return associate_rdma_function<RDMA_TypeType::Get>(han, fn, any_tag, tag);
  }

  RDMA_HandlerType associate_put_function(
    RDMA_HandleType const& han, RDMA_PutFunctionType const& fn,
    bool const& any_tag = false, TagType const& tag = no_tag
  ) {
    return associate_rdma_function<RDMA_TypeType::Put>(han, fn, any_tag, tag);
  }

  void new_channel(
    RDMA_TypeType const& type, RDMA_HandleType const& han, NodeType const& target,
    NodeType const& non_target, ActionType const& action
  );

  void new_get_channel(
    RDMA_HandleType const& han, RDMA_EndpointType const& target,
    RDMA_EndpointType const& non_target, ActionType const& action = nullptr
  ) {
    return new_channel(
      RDMA_TypeType::Get, han, target.get(), non_target.get(), action
    );
  }

  void new_get_channel(
    RDMA_HandleType const& han, NodeType const& target,
    NodeType const& non_target, ActionType const& action = nullptr
  ) {
    return new_channel(
      RDMA_TypeType::Get, han, target, non_target, action
    );
  }

  void new_put_channel(
    RDMA_HandleType const& han, NodeType const& target,
    NodeType const& non_target, ActionType const& action = nullptr
  ) {
    return new_channel(
      RDMA_TypeType::Put, han, target, non_target, action
    );
  }

  void sync_local_get_channel(
    RDMA_HandleType const& han, ActionType const& action
  ) {
    return sync_local_get_channel(han, uninitialized_destination, action);
  }

  void sync_local_get_channel(
    RDMA_HandleType const& han, NodeType const& in_target,
    ActionType const& action = nullptr
  ) {
    auto const& this_node = theContext->get_node();
    auto const& target = get_target(han, in_target);
    bool const is_local = true;
    assert(
      this_node != target and "Sync get works with non-target"
    );
    return sync_channel(is_local, han, RDMA_TypeType::Get, target, this_node, action);
  }

  void sync_local_put_channel(
    RDMA_HandleType const& han, NodeType const& dest, ActionType const& action = nullptr
  ) {
    return sync_local_put_channel(han, dest, uninitialized_destination, action);
  }

  void sync_local_put_channel(
    RDMA_HandleType const& han, NodeType const& dest,
    NodeType const& in_target, ActionType const& action = nullptr
  ) {
    auto const& target = get_target(han, in_target);
    bool const is_local = true;
    return sync_channel(is_local, han, RDMA_TypeType::Put, target, dest, action);
  }

  void sync_remote_get_channel(
    RDMA_HandleType const& han, NodeType const& in_target = uninitialized_destination,
    ActionType const& action = nullptr
  ) {
    auto const& this_node = theContext->get_node();
    auto const& target = get_target(han, in_target);
    bool const is_local = false;
    assert(
      this_node != target and "Sync get works with non-target"
    );
    return sync_channel(is_local, han, RDMA_TypeType::Get, target, this_node, action);
  }

  void sync_remote_put_channel(
    RDMA_HandleType const& han, ActionType const& action
  ) {
    return sync_remote_put_channel(han, uninitialized_destination, action);
  }

  void sync_remote_put_channel(
    RDMA_HandleType const& han, NodeType const& in_target,
    ActionType const& action = nullptr
  ) {
    auto const& this_node = theContext->get_node();
    auto const& target = get_target(han, in_target);
    bool const is_local = false;
    assert(
      this_node != target and "Sync remote put channel should be other target"
    );
    return sync_channel(is_local, han, RDMA_TypeType::Put, target, this_node, action);
  }

  void remove_direct_channel(
    RDMA_HandleType const& han, NodeType const& override_target = uninitialized_destination,
    ActionType const& action = nullptr
  );

private:
  static NodeType get_target(
    RDMA_HandleType const& han, NodeType const& in_tar = uninitialized_destination
  ) {
    auto const target = in_tar == uninitialized_destination ?
      RDMA_HandleManagerType::get_rdma_node(han) : in_tar;
    return target;
  }

  void sync_channel(
    bool const& is_local, RDMA_HandleType const& han, RDMA_TypeType const& type,
    NodeType const& target, NodeType const& non_target, ActionType const& action
  );

  void setup_channel_with_remote(
    RDMA_TypeType const& type, RDMA_HandleType const& han, NodeType const& dest,
    ActionType const& action,
    NodeType const& override_target = uninitialized_destination
  );

  void send_data_channel(
    RDMA_TypeType const& type, RDMA_HandleType const& han, RDMA_PtrType const& ptr,
    ByteType const& num_bytes, ByteType const& offset, NodeType const& target,
    NodeType const& non_target, ActionType cont, ActionType action_after_put
  );

  void create_direct_channel(
    RDMA_TypeType const& type, RDMA_HandleType const& han,
    ActionType const& action = nullptr,
    NodeType const& override_target = uninitialized_destination
  );

  void create_direct_channel_internal(
    RDMA_TypeType const& type, RDMA_HandleType const& han, NodeType const& non_target,
    ActionType const& action = nullptr,
    NodeType const& override_target = uninitialized_destination,
    TagType const& channel_tag = no_tag, ByteType const& num_bytes = no_byte
  );

  void create_direct_channel_finish(
    RDMA_TypeType const& type, RDMA_HandleType const& han, NodeType const& non_target,
    ActionType const& action, TagType const& channel_tag, bool const& is_target,
    ByteType const& num_bytes,
    NodeType const& override_target = uninitialized_destination
  );

  template <RDMAManager::RDMA_TypeType rdma_type, typename FunctionT>
  RDMA_HandlerType associate_rdma_function(
    RDMA_HandleType const& han, FunctionT const& fn, bool const& any_tag,
    TagType const& tag
  ) {
    auto const& this_node = theContext->get_node();
    auto const handler_node = RDMA_HandleManagerType::get_rdma_node(han);
    auto const& is_collective = RDMA_HandleManagerType::is_collective(han);

    assert(
      (is_collective or handler_node == this_node)
      and "Handle must be local to this node"
    );

    auto holder_iter = holder_.find(han);
    assert(
      holder_iter != holder_.end() and "Holder for handler must exist here"
    );

    auto& state = holder_iter->second;

    return state.template set_rdma_fn<rdma_type, FunctionT>(fn, any_tag, tag);
  }

  void request_get_data(
    GetMessage* msg, bool const& is_user_msg,
    RDMA_HandleType const& rdma_handle, TagType const& tag,
    ByteType const& num_bytes, ByteType const& offset,
    RDMA_PtrType const& ptr = nullptr, RDMA_ContinuationType cont = no_action,
    ActionType next_action = no_action
  );

  void trigger_get_recv_data(
    RDMA_OpType const& op, TagType const& tag, RDMA_PtrType ptr,
    ByteType const& num_bytes, ActionType const& action = no_action
  );

  void trigger_put_recv_data(
    RDMA_HandleType const& han, TagType const& tag, RDMA_PtrType ptr,
    ByteType const& num_bytes, ByteType const& offset, ActionType const& action
  );

  RDMA_DirectType try_get_data_ptr_direct(RDMA_OpType const& op);

  RDMA_PtrType try_put_ptr(RDMA_HandleType const& han, TagType const& tag);

  void trigger_put_back_data(RDMA_OpType const& op);

  TagType next_rdma_channel_tag();

  ByteType lookup_bytes_handler(RDMA_HandleType const& han);

  RDMA_ChannelLookupType make_channel_lookup(
    RDMA_HandleType const& han, RDMA_TypeType const& rdma_op_type,
    NodeType const& target, NodeType const& non_target
  );

  RDMA_ChannelType* find_channel(
    RDMA_HandleType const& han, RDMA_TypeType const& rdma_op_type,
    NodeType const& target, NodeType const& non_target,
    bool const& should_insert = false, bool const& must_exist = false
  );

public:
  RDMA_HandlerType allocate_new_rdma_handler();

  // handler functions for managing rdma operations
  static void get_msg(GetMessage* msg);
  static void get_recv_msg(GetBackMessage* msg);
  static void put_back_msg(PutBackMessage* msg);
  static void put_recv_msg(PutMessage* msg);

  // handler functions for managing direct rdma channels
  static void setup_channel(CreateChannel* msg);
  static void remove_channel(DestroyChannel* msg);
  static void remote_channel(ChannelMessage* msg);
  static void get_info_channel(GetInfoChannel* msg);

private:
  // next local rdma handler (used by State)
  RDMA_HandlerType cur_rdma_handler_ = first_rdma_handler;

  // next local rdma identifier
  RDMA_IdentifierType cur_ident_ = first_rdma_identifier;

  // next collective rdma identifier
  RDMA_IdentifierType cur_collective_ident_ = first_rdma_identifier;

  // rdma state container
  RDMA_ContainerType holder_;

  // rdma unique remote operation identifier
  RDMA_OpType cur_op_ = 0;

  // operations that are pending remote interaction
  RDMA_OpContainerType pending_ops_;

  // Live channels that can be used to hardware-level get/put ops
  RDMA_LiveChannelsType channels_;

  TagType next_channel_tag_ = first_rdma_channel_tag;
};

}} //end namespace vt::rdma

namespace vt {

extern std::unique_ptr<rdma::RDMAManager> theRDMA;

} //end namespace vt

#endif /*__RUNTIME_TRANSPORT_RDMA__*/
