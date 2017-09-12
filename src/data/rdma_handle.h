
#if ! defined __RUNTIME_TRANSPORT_RDMAHANDLER__
#define __RUNTIME_TRANSPORT_RDMAHANDLER__

#include "common.h"
#include "rdma_common.h"

namespace vt { namespace rdma {

static_assert(
  sizeof(RDMA_HandleType) == sizeof(RDMA_HandlerType),
  "RDMA Handle and RDMA Handler IDs must be the same size"
);

struct HandleManager {
  using RDMA_BitsType = Bits;
  using RDMA_TypeType = Type;
  using RDMA_UniversalIdType = RDMA_HandleType;

  static void set_is_sized(RDMA_UniversalIdType& handle, bool const& is_sized);
  static void set_is_collective(RDMA_UniversalIdType& handle, bool const& is_collective);
  static void set_is_handler(
    RDMA_UniversalIdType& handle, bool const& is_handler
  );
  static void set_op_type(
    RDMA_UniversalIdType& handle, RDMA_TypeType const& rdma_type
  );
  static void set_rdma_node(RDMA_UniversalIdType& handle, NodeType const& node);
  static void set_rdma_identifier(
    RDMA_UniversalIdType& handle, RDMA_IdentifierType const& ident
  );
  static NodeType get_rdma_node(RDMA_UniversalIdType const& handle);
  static RDMA_IdentifierType get_rdma_identifier(RDMA_UniversalIdType const& handle);
  static bool is_sized(RDMA_UniversalIdType const& handle);
  static bool is_collective(RDMA_UniversalIdType const& handle);
  static bool is_handler(RDMA_UniversalIdType const& handle);
  static RDMA_TypeType get_op_type(RDMA_UniversalIdType const& handle);
  static RDMA_UniversalIdType create_new_handler();
};

using RDMA_HandleManagerType = HandleManager;

}} //end namespace vt::rdma

#endif /*__RUNTIME_TRANSPORT_RDMAHANDLER__*/
