
#include "rdma_handle.h"

namespace runtime { namespace rdma {

/*static*/ void
RDMAHandleManager::set_rdma_bit(
  universal_rdma_id_t& handle, bool const& set, rdma_bits_t const& bit
) {
  if (set) {
    handle |= 1 << bit;
  } else {
    handle &= ~(1 << bit);
  }
}

/*static*/ void
RDMAHandleManager::set_is_sized(
  universal_rdma_id_t& handle, bool const& is_sized
) {
  return set_rdma_bit(handle, is_sized, rdma_bits_t::Sized);
}

/*static*/ void
RDMAHandleManager::set_is_collective(
  universal_rdma_id_t& handle, bool const& is_collective
) {
  return set_rdma_bit(handle, is_collective, rdma_bits_t::Collective);
}

/*static*/ void
RDMAHandleManager::set_is_handler(
  universal_rdma_id_t& handle, bool const& is_handler
) {
  return set_rdma_bit(handle, is_handler, rdma_bits_t::HandlerType);
}

/*static*/ void
RDMAHandleManager::set_op_type(
  universal_rdma_id_t& handle, rdma_type_t const& rdma_type
) {
  bool const op_set = rdma_type == rdma_type_t::Put;
  set_rdma_bit(handle, op_set, rdma_bits_t::OpType);
}

/*static*/ void
RDMAHandleManager::set_rdma_node(
  universal_rdma_id_t& handle, node_t const& node
) {
  handle |= node << rdma_bits_t::Node;
}

/*static*/ void
RDMAHandleManager::set_rdma_identifier(
  universal_rdma_id_t& handle, rdma_identifier_t const& ident
) {
  handle |= ident << rdma_bits_t::Identifier;
}

/*static*/ node_t
RDMAHandleManager::get_rdma_node(universal_rdma_id_t const& handle) {
  node_t const rdma_node = static_cast<node_t>(handle >> rdma_bits_t::Node);
  return rdma_node;
}

/*static*/ rdma_identifier_t
RDMAHandleManager::get_rdma_identifier(universal_rdma_id_t const& handle) {
  rdma_identifier_t const ident = static_cast<rdma_identifier_t>(
    handle >> rdma_bits_t::Identifier
  );
  return ident;
}

/*static*/ bool
RDMAHandleManager::is_sized(universal_rdma_id_t const& handle) {
  return (1 << rdma_bits_t::Sized) & handle;
}

/*static*/ bool
RDMAHandleManager::is_collective(universal_rdma_id_t const& handle) {
  return (1 << rdma_bits_t::Collective) & handle;
}

/*static*/ bool
RDMAHandleManager::is_handler(universal_rdma_id_t const& handle) {
  return (1 << rdma_bits_t::HandlerType) & handle;
}

/*static*/ RDMAHandleManager::rdma_type_t
RDMAHandleManager::get_op_type(universal_rdma_id_t const& handle) {
  if ((1 << rdma_bits_t::OpType) & (handle == rdma_type_t::Put)) {
    return rdma_type_t::Put;
  } else {
    return rdma_type_t::Get;
  }
}

}} // end namespace runtime::rdma

