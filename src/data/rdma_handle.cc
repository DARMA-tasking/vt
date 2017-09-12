
#include "rdma_handle.h"
#include "bit_common.h"

namespace runtime { namespace rdma {

/*static*/ void HandleManager::set_is_sized(
  RDMA_UniversalIdType& handle, bool const& is_sized
) {
  BitPackerType::bool_set_field<RDMA_BitsType::Sized>(handle, is_sized);
}

/*static*/ void HandleManager::set_is_collective(
  RDMA_UniversalIdType& handle, bool const& is_collective
) {
  BitPackerType::bool_set_field<RDMA_BitsType::Collective>(handle, is_collective);
}

/*static*/ void HandleManager::set_is_handler(
  RDMA_UniversalIdType& handle, bool const& is_handler
) {
  BitPackerType::bool_set_field<RDMA_BitsType::HandlerType>(handle, is_handler);
}

/*static*/ void HandleManager::set_op_type(
  RDMA_UniversalIdType& handle, RDMA_TypeType const& rdma_type
) {
  BitPackerType::set_field<RDMA_BitsType::OpType, rdma_type_num_bits>(
    handle, rdma_type
  );
}

/*static*/ void HandleManager::set_rdma_node(
  RDMA_UniversalIdType& handle, NodeType const& node
) {
  BitPackerType::set_field<RDMA_BitsType::Node, node_num_bits>(handle, node);
}

/*static*/ void HandleManager::set_rdma_identifier(
  RDMA_UniversalIdType& handle, RDMA_IdentifierType const& ident
) {
  BitPackerType::set_field<RDMA_BitsType::Identifier, rdma_identifier_num_bits>(
    handle, ident
  );
}

/*static*/ NodeType HandleManager::get_rdma_node(
  RDMA_UniversalIdType const& handle
) {
  return BitPackerType::get_field<RDMA_BitsType::Node, node_num_bits, NodeType>(handle);
}

/*static*/ RDMA_IdentifierType HandleManager::get_rdma_identifier(
  RDMA_UniversalIdType const& handle
) {
  return BitPackerType::get_field<
    RDMA_BitsType::Identifier, rdma_identifier_num_bits, RDMA_IdentifierType
  >(handle);
}

/*static*/ bool HandleManager::is_sized(RDMA_UniversalIdType const& handle) {
  return BitPackerType::bool_get_field<RDMA_BitsType::Sized>(handle);
}

/*static*/ bool HandleManager::is_collective(RDMA_UniversalIdType const& handle) {
  return BitPackerType::bool_get_field<RDMA_BitsType::Collective>(handle);
}

/*static*/ bool HandleManager::is_handler(RDMA_UniversalIdType const& handle) {
  return BitPackerType::bool_get_field<RDMA_BitsType::HandlerType>(handle);
}

/*static*/ HandleManager::RDMA_TypeType HandleManager::get_op_type(
  RDMA_UniversalIdType const& handle
) {
  return BitPackerType::get_field<
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

