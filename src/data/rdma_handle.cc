
#include "rdma_handle.h"

namespace runtime { namespace rdma {

/*static*/ void
HandleManager::set_rdma_bit(
  universal_rdma_id_t& handle, bool const& set, rdma_bits_t const& bit
) {
  if (set) {
    handle |= 1 << bit;
  } else {
    handle &= ~(1 << bit);
  }
}

/*static*/ void
HandleManager::set_is_sized(
  universal_rdma_id_t& handle, bool const& is_sized
) {
  return set_rdma_bit(handle, is_sized, rdma_bits_t::Sized);
}

/*static*/ void
HandleManager::set_is_collective(
  universal_rdma_id_t& handle, bool const& is_collective
) {
  return set_rdma_bit(handle, is_collective, rdma_bits_t::Collective);
}

/*static*/ void
HandleManager::set_is_handler(
  universal_rdma_id_t& handle, bool const& is_handler
) {
  return set_rdma_bit(handle, is_handler, rdma_bits_t::HandlerType);
}

/*static*/ void
HandleManager::set_op_type(
  universal_rdma_id_t& handle, rdma_type_t const& rdma_type
) {
  handle = 0xFFFFFFC7 & handle;
  handle |= rdma_type << rdma_bits_t::OpType;
}

/*static*/ void
HandleManager::set_rdma_node(
  universal_rdma_id_t& handle, node_t const& node
) {
  handle |= (universal_rdma_id_t)node << rdma_bits_t::Node;
}

/*static*/ void
HandleManager::set_rdma_identifier(
  universal_rdma_id_t& handle, rdma_identifier_t const& ident
) {
  handle |= (universal_rdma_id_t)ident << rdma_bits_t::Identifier;
}

/*static*/ node_t
HandleManager::get_rdma_node(universal_rdma_id_t const& handle) {
  node_t const rdma_node = static_cast<node_t>(handle >> rdma_bits_t::Node);
  return rdma_node;
}

/*static*/ rdma_identifier_t
HandleManager::get_rdma_identifier(universal_rdma_id_t const& handle) {
  rdma_identifier_t const ident = static_cast<rdma_identifier_t>(
    handle >> rdma_bits_t::Identifier
  );
  return ident;
}

/*static*/ bool
HandleManager::is_sized(universal_rdma_id_t const& handle) {
  return (1 << rdma_bits_t::Sized) & handle;
}

/*static*/ bool
HandleManager::is_collective(universal_rdma_id_t const& handle) {
  return (1 << rdma_bits_t::Collective) & handle;
}

/*static*/ bool
HandleManager::is_handler(universal_rdma_id_t const& handle) {
  return (1 << rdma_bits_t::HandlerType) & handle;
}

/*static*/ HandleManager::rdma_type_t
HandleManager::get_op_type(universal_rdma_id_t const& handle) {
  rdma_type_t const type = static_cast<rdma_type_t>(
    (0x00000038 & handle) >> rdma_bits_t::OpType
  );
  return type;
}

/*static*/ HandleManager::universal_rdma_id_t
HandleManager::create_new_handler() {
  rdma_type_t const& type = rdma_type_t::Uninitialized;
  rdma_handle_t handle = 0;
  set_op_type(handle, type);
  return handle;
}


}} // end namespace runtime::rdma

