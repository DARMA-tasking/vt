
#if ! defined __RUNTIME_TRANSPORT_COMMON_TYPE_SENTINELS__
#define __RUNTIME_TRANSPORT_COMMON_TYPE_SENTINELS__

#include "utils/debug/debug_masterconfig.h"
#include "common_types.h"

namespace vt {

static constexpr int const num_check_actions = 8;

static constexpr int const mpi_event_tag = 0;
static constexpr int const normal_event_tag = 1;

static constexpr EpochType const no_epoch = -1;
static constexpr TagType const no_tag = -1;
static constexpr EventType const no_event = -1;
static constexpr BarrierType const no_barrier = -1;
static constexpr RDMA_HandleType const no_rdma_handle = -1;
static constexpr ByteType const no_byte = -1;
static constexpr ByteType const no_offset = -1;
static constexpr auto no_action = nullptr;
static constexpr RDMA_PtrType const no_rdma_ptr = nullptr;

static constexpr NodeType const uninitialized_destination = 0xFFFF;
static constexpr HandlerType const uninitialized_handler = -1;
static constexpr RDMA_HandlerType const uninitialized_rdma_handler = -1;
static constexpr RefType const not_shared_message = -1000;
static constexpr RDMA_ElmType const no_rdma_elm = -1;
static constexpr RDMA_BlockType const no_rdma_block = -1;

} //end namespace vt

#endif /*__RUNTIME_TRANSPORT_EVENT_COMMON_TYPE_SENTINELS__*/
