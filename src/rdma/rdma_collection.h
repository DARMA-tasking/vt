
#if !defined INCLUDED_RDMA_RDMA_COLLECTION_H
#define INCLUDED_RDMA_RDMA_COLLECTION_H

#include "configs/types/types_rdma.h"
#include "rdma/rdma_collection_fwd.h"
#include "rdma/rdma_collection_info.h"
#include "rdma/rdma.h"

#include <functional>

namespace vt { namespace rdma {

struct RDMACollectionManager {
  using RDMA_ActionPtr = std::function<void(RDMA_PtrType const& ptr)>;

  static RDMA_HandleType registerUnsizedCollection(
    RDMA_ElmType const& num_elms,
    RDMAManager::RDMA_MapType const& map = default_map,
    bool const& demand_allocate = true
  );

  static void getElement(
    RDMA_HandleType const& rdma_handle,
    RDMA_ElmType const& elm,
    RDMA_RecvType action_ptr = nullptr,
    TagType const& tag = no_tag
  );

  static void putElement(
    RDMA_HandleType const& rdma_handle,
    RDMA_ElmType const& elm,
    RDMA_PtrType const& ptr,
    RDMA_PutSerialize on_demand_put_serialize = no_action,
    ActionType cont = no_action,
    ActionType action_after_put = no_action,
    TagType const& tag = no_tag
  );
};

}} /* end namespace vt::rdma */

#endif /*INCLUDED_RDMA_RDMA_COLLECTION_H*/
