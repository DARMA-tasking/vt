
#if ! defined __RUNTIME_TRANSPORT_COMMON__
#define __RUNTIME_TRANSPORT_COMMON__

#include <cstdint>
#include <functional>

#include "debug.h"

namespace runtime {

using node_t = int16_t;
using handler_t = int16_t;
using envelope_datatype_t = int8_t;
using event_t = uint64_t;

struct Message;

using active_function_t = std::function<void(Message*)>;
using action_t = std::function<void()>;

constexpr int const num_check_actions = 8;
constexpr int const scheduler_default_num_times = 1;

static constexpr int const mpi_event_tag = 0;
static constexpr int const normal_event_tag = 1;

static constexpr node_t const broadcast_dest_sentinel = -1;

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_EVENT_COMMON__*/
