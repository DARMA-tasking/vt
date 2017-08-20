
#if ! defined __RUNTIME_TRANSPORT_COMMON__
#define __RUNTIME_TRANSPORT_COMMON__

#include <cstdint>
#include <functional>
#include <cassert>

#include "debug.h"

namespace runtime {

using node_t = int16_t;
using handler_t = int16_t;
using envelope_datatype_t = int8_t;
using event_t = uint64_t;
using epoch_t = int32_t;
using tag_t = int32_t;
using barrier_t = uint64_t;
using ref_t = int16_t;
using byte_t = uint64_t;
using action_t = std::function<void()>;
using rdma_ptr_t = void*;
using rdma_handle_t = int64_t;
using rdma_handler_t = int64_t;
using rdma_get_t = std::tuple<rdma_ptr_t, byte_t>;
using rdma_continuation_t = std::function<void(rdma_get_t)>;
using rdma_continuation_del_t = std::function<void(rdma_get_t, action_t)>;

constexpr int const num_check_actions = 8;
constexpr int const scheduler_default_num_times = 1;

static constexpr int const mpi_event_tag = 0;
static constexpr int const normal_event_tag = 1;

static constexpr epoch_t const no_epoch = -1;
static constexpr tag_t const no_tag = -1;
static constexpr event_t const no_event = -1;
static constexpr barrier_t const no_barrier = -1;
static constexpr rdma_handle_t const no_rdma_handle = -1;
static constexpr byte_t const no_byte = -1;
static constexpr auto no_action = nullptr;
static constexpr rdma_ptr_t const no_rdma_ptr = nullptr;

static constexpr node_t const uninitialized_destination = -1;
static constexpr handler_t const uninitialized_handler = -1;
static constexpr rdma_handler_t const uninitialized_rdma_handler = -1;
static constexpr ref_t const not_shared_message = -1000;

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_EVENT_COMMON__*/
