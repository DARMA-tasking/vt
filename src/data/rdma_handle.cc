
#include "rdma_handle.h"
#include "bit_common.h"

namespace runtime { namespace rdma {

/*static*/ void HandleManager::set_is_sized(
  RDMA_UniversalIdType& handle, bool const& is_sized
) {
  bit_packer_t::bool_set_field<RDMA_BitsType::Sized>(handle, is_sized);
}

/*static*/ void HandleManager::set_is_collective(
  RDMA_UniversalIdType& handle, bool const& is_collective
) {
  bit_packer_t::bool_set_field<RDMA_BitsType::Collective>(handle, is_collective);
}

/*static*/ void HandleManager::set_is_handler(
  RDMA_UniversalIdType& handle, bool const& is_handler
) {
  bit_packer_t::bool_set_field<RDMA_BitsType::HandlerType>(handle, is_handler);
}

/*static*/ void HandleManager::set_op_type(
  RDMA_UniversalIdType& handle, RDMA_TypeType const& rdma_type
) {
  bit_packer_t::set_field<RDMA_BitsType::OpType, rdma_type_num_bits>(
    handle, rdma_type
  );
}

/*static*/ void HandleManager::set_rdma_node(
  RDMA_UniversalIdType& handle, NodeType const& node
) {
  bit_packer_t::set_field<RDMA_BitsType::Node, node_num_bits>(handle, node);
}

/*static*/ void HandleManager::set_rdma_identifier(
  RDMA_UniversalIdType& handle, RDMA_IdentifierType const& ident
) {
  bit_packer_t::set_field<RDMA_BitsType::Identifier, rdma_identifier_num_bits>(
    handle, ident
  );
}

/*static*/ NodeType HandleManager::get_rdma_node(
  RDMA_UniversalIdType const& handle
) {
  return bit_packer_t::get_field<RDMA_BitsType::Node, node_num_bits, NodeType>(handle);
}

/*static*/ RDMA_IdentifierType HandleManager::get_rdma_identifier(
  RDMA_UniversalIdType const& handle
) {
  return bit_packer_t::get_field<
    RDMA_BitsType::Identifier, rdma_identifier_num_bits, RDMA_IdentifierType
  >(handle);
}

/*static*/ bool HandleManager::is_sized(RDMA_UniversalIdType const& handle) {
  return bit_packer_t::bool_get_field<RDMA_BitsType::Sized>(handle);
}

/*static*/ bool HandleManager::is_collective(RDMA_UniversalIdType const& handle) {
  return bit_packer_t::bool_get_field<RDMA_BitsType::Collective>(handle);
}

/*static*/ bool HandleManager::is_handler(RDMA_UniversalIdType const& handle) {
  return bit_packer_t::bool_get_field<RDMA_BitsType::HandlerType>(handle);
}

/*static*/ HandleManager::RDMA_TypeType HandleManager::get_op_type(
  RDMA_UniversalIdType const& handle
) {
  return bit_packer_t::get_field<
    RDMA_BitsType::OpType, rdma_type_num_bits, RDMA_TypeType
  >(handle);
}

/*static*/ HandleManager::RDMA_UniversalIdType
HandleManager::create_new_handler() {
  RDMA_TypeType const& type = RDMA_TypeType::Uninitialized;
  RDMA_HandleType handle = 0;
  set_op_type(handle, type);
  return handle;
}

}} // end namespace runtime::rdma

