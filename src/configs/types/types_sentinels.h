
#if !defined INCLUDED_TYPES_SENTINELS
#define INCLUDED_TYPES_SENTINELS

#include "configs/debug/debug_masterconfig.h"
#include "types_type.h"

namespace vt {

// Physical identifier sentinel values (nodes, cores, workers, etc.)
static constexpr NodeType const uninitialized_destination = 0xFFFF;
static constexpr WorkerCountType const no_workers = 0xFFFF;
static constexpr WorkerIDType const no_worker_id = 0xFFFE;
static constexpr WorkerIDType const worker_id_comm_thread = 0xFEED;
static constexpr WorkerIDType const comm_debug_print = -1;

// Runtime identifier sentinel values
static constexpr int const num_check_actions = 8;
static constexpr EpochType const no_epoch = -1;
static constexpr TagType const no_tag = -1;
static constexpr EventType const no_event = -1;
static constexpr BarrierType const no_barrier = -1;
static constexpr RDMA_HandleType const no_rdma_handle = -1;
static constexpr ByteType const no_byte = -1;
static constexpr ByteType const no_offset = -1;
static constexpr auto no_action = nullptr;
static constexpr RDMA_PtrType const no_rdma_ptr = nullptr;
static constexpr VirtualProxyType const no_vrt_proxy = 0xFFFFFFFF;
static constexpr HandlerType const uninitialized_handler = -1;
static constexpr RDMA_HandlerType const uninitialized_rdma_handler = -1;
static constexpr RefType const not_shared_message = -1000;
static constexpr RDMA_ElmType const no_rdma_elm = -1;
static constexpr RDMA_BlockType const no_rdma_block = -1;
static constexpr SeedType const no_seed = -1;
static constexpr VirtualElmCountType const no_elms = -1;
static constexpr TagType const local_rdma_op_tag = 0xFEEDFEED;


}  // end namespace vt

#endif  /*INCLUDED_TYPES_SENTINELS*/
