
#include "rdma_handle.h"
#include "bit_common.h"

namespace vt { namespace rdma {

/*static*/ void HandleManager::setIsSized(
  RDMA_UniversalIdType& handle, bool const& is_sized
) {
  BitPackerType::boolSetField<RDMA_BitsType::Sized>(handle, is_sized);
}

/*static*/ void HandleManager::setIsCollective(
  RDMA_UniversalIdType& handle, bool const& is_collective
) {
  BitPackerType::boolSetField<RDMA_BitsType::Collective>(handle, is_collective);
}

/*static*/ void HandleManager::setIsHandler(
  RDMA_UniversalIdType& handle, bool const& is_handler
) {
  BitPackerType::boolSetField<RDMA_BitsType::HandlerType>(handle, is_handler);
}

/*static*/ void HandleManager::setOpType(
  RDMA_UniversalIdType& handle, RDMA_TypeType const& rdma_type
) {
  BitPackerType::setField<RDMA_BitsType::OpType, rdma_type_num_bits>(
    handle, rdma_type
  );
}

/*static*/ void HandleManager::setRdmaNode(
  RDMA_UniversalIdType& handle, NodeType const& node
) {
  BitPackerType::setField<RDMA_BitsType::Node, node_num_bits>(handle, node);
}

/*static*/ void HandleManager::setRdmaIdentifier(
  RDMA_UniversalIdType& handle, RDMA_IdentifierType const& ident
) {
  BitPackerType::setField<RDMA_BitsType::Identifier, rdma_identifier_num_bits>(
    handle, ident
  );
}

/*static*/ NodeType HandleManager::getRdmaNode(
  RDMA_UniversalIdType const& handle
) {
  return BitPackerType::getField<RDMA_BitsType::Node, node_num_bits, NodeType>(handle);
}

/*static*/ RDMA_IdentifierType HandleManager::getRdmaIdentifier(
  RDMA_UniversalIdType const& handle
) {
  return BitPackerType::getField<
    RDMA_BitsType::Identifier, rdma_identifier_num_bits, RDMA_IdentifierType
  >(handle);
}

/*static*/ bool HandleManager::isSized(RDMA_UniversalIdType const& handle) {
  return BitPackerType::boolGetField<RDMA_BitsType::Sized>(handle);
}

/*static*/ bool HandleManager::isCollective(RDMA_UniversalIdType const& handle) {
  return BitPackerType::boolGetField<RDMA_BitsType::Collective>(handle);
}

/*static*/ bool HandleManager::isHandler(RDMA_UniversalIdType const& handle) {
  return BitPackerType::boolGetField<RDMA_BitsType::HandlerType>(handle);
}

/*static*/ HandleManager::RDMA_TypeType HandleManager::getOpType(
  RDMA_UniversalIdType const& handle
) {
  return BitPackerType::getField<
    RDMA_BitsType::OpType, rdma_type_num_bits, RDMA_TypeType
  >(handle);
}

/*static*/ HandleManager::RDMA_UniversalIdType
HandleManager::createNewHandler() {
  RDMA_TypeType const& type = RDMA_TypeType::Uninitialized;
  RDMA_HandleType handle = 0;
  setOpType(handle, type);
  return handle;
}

}} // end namespace vt::rdma

