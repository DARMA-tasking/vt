
#if !defined INCLUDED_TYPES_RDMA
#define INCLUDED_TYPES_RDMA

#include "configs/debug/debug_masterconfig.h"
#include "configs/types/types_type.h"

#include <cstdint>
#include <functional>
#include <tuple>

namespace vt {

using RDMA_PtrType                = void *;
using RDMA_ElmType                = int64_t;
using RDMA_BlockType              = int64_t;
using RDMA_HandleType             = int64_t;
using RDMA_HandlerType            = int64_t;
using RDMA_GetType                = std::tuple<RDMA_PtrType, ByteType>;
using RDMA_PutRetType             = RDMA_GetType;
using RDMA_ContinuationType       = std::function<void(RDMA_GetType)>;
using RDMA_ContinuationDeleteType = std::function<
  void(RDMA_GetType, ActionType)
>;
using RDMA_PutSerialize = std::function<RDMA_PutRetType(RDMA_PutRetType)>;

}  // end namespace vt

#endif  /*INCLUDED_TYPES_RDMA*/
