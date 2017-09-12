
#include "rdma_handle.h"
#include "bit_common.h"

namespace runtime { namespace rdma {

/*static*/ void
HandleManager::set_is_sized(
  universal_rdma_id_t& handle, bool const& is_sized
) {
  bit_packer_t::bool_set_field<rdma_bits_t::Sized>(handle, is_sized);
}

/*static*/ void
HandleManager::set_is_collective(
  universal_rdma_id_t& handle, bool const& is_collective
) {
  bit_packer_t::bool_set_field<rdma_bits_t::Collective>(handle, is_collective);
}

/*static*/ void
HandleManager::set_is_handler(
  universal_rdma_id_t& handle, bool const& is_handler
) {
  bit_packer_t::bool_set_field<rdma_bits_t::HandlerType>(handle, is_handler);
}

/*static*/ void
HandleManager::set_op_type(
  universal_rdma_id_t& handle, rdma_type_t const& rdma_type
) {
  bit_packer_t::set_field<rdma_bits_t::OpType, rdma_type_num_bits>(
    handle, rdma_type
  );
}

/*static*/ void
HandleManager::set_rdma_node(
  universal_rdma_id_t& handle, NodeType const& node
) {
  bit_packer_t::set_field<rdma_bits_t::Node, node_num_bits>(handle, node);
}

/*static*/ void
HandleManager::set_rdma_identifier(
  universal_rdma_id_t& handle, rdma_identifier_t const& ident
) {
  bit_packer_t::set_field<rdma_bits_t::Identifier, rdma_identifier_num_bits>(
    handle, ident
  );
}

/*static*/ NodeType
HandleManager::get_rdma_node(universal_rdma_id_t const& handle) {
  return bit_packer_t::get_field<rdma_bits_t::Node, node_num_bits, NodeType>(handle);
}

/*static*/ rdma_identifier_t
HandleManager::get_rdma_identifier(universal_rdma_id_t const& handle) {
  return bit_packer_t::get_field<
    rdma_bits_t::Identifier, rdma_identifier_num_bits, rdma_identifier_t
  >(handle);
}

/*static*/ bool
HandleManager::is_sized(universal_rdma_id_t const& handle) {
  return bit_packer_t::bool_get_field<rdma_bits_t::Sized>(handle);
}

/*static*/ bool
HandleManager::is_collective(universal_rdma_id_t const& handle) {
  return bit_packer_t::bool_get_field<rdma_bits_t::Collective>(handle);
}

/*static*/ bool
HandleManager::is_handler(universal_rdma_id_t const& handle) {
  return bit_packer_t::bool_get_field<rdma_bits_t::HandlerType>(handle);
}

/*static*/ HandleManager::rdma_type_t
HandleManager::get_op_type(universal_rdma_id_t const& handle) {
  return bit_packer_t::get_field<
    rdma_bits_t::OpType, rdma_type_num_bits, rdma_type_t
  >(handle);
}

/*static*/ HandleManager::universal_rdma_id_t
HandleManager::create_new_handler() {
  rdma_type_t const& type = rdma_type_t::Uninitialized;
  RDMA_HandleType handle = 0;
  set_op_type(handle, type);
  return handle;
}


}} // end namespace runtime::rdma

