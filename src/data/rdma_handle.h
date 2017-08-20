
#if ! defined __RUNTIME_TRANSPORT_RDMAHANDLER__
#define __RUNTIME_TRANSPORT_RDMAHANDLER__

#include "common.h"
#include "rdma_common.h"

namespace runtime { namespace rdma {

static_assert(
  sizeof(rdma_handle_t) == sizeof(rdma_handler_t),
  "RDMA Handle and RDMA Handler IDs must be the same size"
);

struct HandleManager {
  using rdma_bits_t = Bits;
  using rdma_type_t = Type;
  using universal_rdma_id_t = rdma_handle_t;

  static void
  set_is_sized(universal_rdma_id_t& handle, bool const& is_sized);

  static void
  set_is_collective(universal_rdma_id_t& handle, bool const& is_collective);

  static void
  set_is_handler(
    universal_rdma_id_t& handle, bool const& is_handler
  );

  static void
  set_op_type(
    universal_rdma_id_t& handle, rdma_type_t const& rdma_type
  );

  static void
  set_rdma_bit(
    universal_rdma_id_t& handle, bool const& set, rdma_bits_t const& bit
  );

  static void
  set_rdma_node(universal_rdma_id_t& handle, node_t const& node);

  static void
  set_rdma_identifier(
    universal_rdma_id_t& handle, rdma_identifier_t const& ident
  );

  static node_t
  get_rdma_node(universal_rdma_id_t const& handle);

  static rdma_identifier_t
  get_rdma_identifier(universal_rdma_id_t const& handle);

  static bool
  is_sized(universal_rdma_id_t const& handle);

  static bool
  is_collective(universal_rdma_id_t const& handle);

  static bool
  is_handler(universal_rdma_id_t const& handle);

  static rdma_type_t
  get_op_type(universal_rdma_id_t const& handle);
};

}} //end namespace runtime::rdma

#endif /*__RUNTIME_TRANSPORT_RDMAHANDLER__*/
