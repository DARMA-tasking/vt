
#if !defined INCLUDED_RDMA_RDMA_HANDLE_H
#define INCLUDED_RDMA_RDMA_HANDLE_H

#include "config.h"
#include "rdma/rdma_common.h"

namespace vt { namespace rdma {

static_assert(
  sizeof(RDMA_HandleType) == sizeof(RDMA_HandlerType),
  "RDMA Handle, RDMA Handler IDs must be the same size"
);

struct HandleManager {
  using RDMA_BitsType = Bits;
  using RDMA_TypeType = Type;
  using RDMA_UniversalIdType = RDMA_HandleType;

  static void setIsSized(RDMA_UniversalIdType& handle, bool const& is_sized);
  static void setIsCollective(RDMA_UniversalIdType& handle, bool const& is_collective);
  static void setIsHandler(
    RDMA_UniversalIdType& handle, bool const& is_handler
  );
  static void setOpType(
    RDMA_UniversalIdType& handle, RDMA_TypeType const& rdma_type
  );
  static void setRdmaNode(RDMA_UniversalIdType& handle, NodeType const& node);
  static void setRdmaIdentifier(
    RDMA_UniversalIdType& handle, RDMA_IdentifierType const& ident
  );
  static NodeType getRdmaNode(RDMA_UniversalIdType const& handle);
  static RDMA_IdentifierType getRdmaIdentifier(RDMA_UniversalIdType const& handle);
  static bool isSized(RDMA_UniversalIdType const& handle);
  static bool isCollective(RDMA_UniversalIdType const& handle);
  static bool isHandler(RDMA_UniversalIdType const& handle);
  static RDMA_TypeType getOpType(RDMA_UniversalIdType const& handle);
  static RDMA_UniversalIdType createNewHandler();
};

using RDMA_HandleManagerType = HandleManager;

}} //end namespace vt::rdma

#endif /*INCLUDED_RDMA_RDMA_HANDLE_H*/
