
#if ! defined __RUNTIME_TRANSPORT_COMMON_RDMA_TYPES__
#define __RUNTIME_TRANSPORT_COMMON_RDMA_TYPES__

#include "config.h"

#include <cstdint>
#include <functional>

namespace runtime {

using RDMA_PtrType = void*;
using RDMA_ElmType = int64_t;
using RDMA_BlockType = int64_t;
using RDMA_HandleType = int64_t;
using RDMA_HandlerType = int64_t;
using RDMA_GetType = std::tuple<RDMA_PtrType, ByteType>;
using RDMA_ContinuationType = std::function<void(RDMA_GetType)>;
using RDMA_ContinuationDeleteType = std::function<void(RDMA_GetType, ActionType)>;

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_EVENT_COMMON_TYPES__*/
