
#if ! defined __RUNTIME_TRANSPORT_RDMAHANDLER__
#define __RUNTIME_TRANSPORT_RDMAHANDLER__

#include "common.h"
#include "rdma_common.h"

namespace runtime { namespace rdma {

static_assert(
  sizeof(RDMA_HandleType) == sizeof(RDMA_HandlerType),
  "RDMA Handle and RDMA Handler IDs must be the same size"
);

struct HandleManager {
  using rdma_bits_t = Bits;
  using RDMA_TypeType = Type;
  using universal_rdma_id_t = RDMA_HandleType;

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
    universal_rdma_id_t& handle, RDMA_TypeType const& rdma_type
  );

  static void
  set_rdma_node(universal_rdma_id_t& handle, NodeType const& node);

  static void
  set_rdma_identifier(
    universal_rdma_id_t& handle, RDMA_IdentifierType const& ident
  );

  static NodeType
  get_rdma_node(universal_rdma_id_t const& handle);

  static RDMA_IdentifierType
  get_rdma_identifier(universal_rdma_id_t const& handle);

  static bool
  is_sized(universal_rdma_id_t const& handle);

  static bool
  is_collective(universal_rdma_id_t const& handle);

  static bool
  is_handler(universal_rdma_id_t const& handle);

  static RDMA_TypeType
  get_op_type(universal_rdma_id_t const& handle);

  static universal_rdma_id_t
  create_new_handler();
};

using RDMA_HandleManagerType = HandleManager;

}} //end namespace runtime::rdma

#endif /*__RUNTIME_TRANSPORT_RDMAHANDLER__*/
