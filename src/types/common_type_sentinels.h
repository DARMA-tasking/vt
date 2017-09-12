
#if ! defined __RUNTIME_TRANSPORT_COMMON_TYPE_SENTINELS__
#define __RUNTIME_TRANSPORT_COMMON_TYPE_SENTINELS__

#include "config.h"
#include "common_types.h"

namespace runtime {

static constexpr int const num_check_actions = 8;

static constexpr int const mpi_event_tag = 0;
static constexpr int const normal_event_tag = 1;

static constexpr EpochType const no_epoch = -1;
static constexpr TagType const no_tag = -1;
static constexpr event_t const no_event = -1;
static constexpr barrier_t const no_barrier = -1;
static constexpr rdma_handle_t const no_rdma_handle = -1;
static constexpr byte_t const no_byte = -1;
static constexpr byte_t const no_offset = -1;
static constexpr auto no_action = nullptr;
static constexpr rdma_ptr_t const no_rdma_ptr = nullptr;

static constexpr NodeType const uninitialized_destination = 0xFFFF;
static constexpr HandlerType const uninitialized_handler = -1;
static constexpr rdma_handler_t const uninitialized_rdma_handler = -1;
static constexpr ref_t const not_shared_message = -1000;
static constexpr rdma_elm_t const no_rdma_elm = -1;
static constexpr rdma_block_t const no_rdma_block = -1;

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_EVENT_COMMON_TYPE_SENTINELS__*/
