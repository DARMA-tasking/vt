
#if ! defined __RUNTIME_TRANSPORT_TRACE_COMMON__
#define __RUNTIME_TRANSPORT_TRACE_COMMON__

#include "common.h"

#include <cstdint>
#include <functional>
#include <string>

namespace runtime { namespace trace {

static constexpr uint32_t const trace_flush_size = 100000;

using trace_ep_t = std::hash<std::string>::result_type;
using trace_event_t = uint32_t;
using trace_msg_len_t = size_t;

static constexpr trace_ep_t const no_trace_ep = -1;
static constexpr trace_event_t const no_trace_event = 0;
static constexpr NodeType const designated_root_node = 0;
static constexpr int64_t const trace_reserve_count = 1048576;

static constexpr bit_count_t const trace_event_num_bits = 32;

}} //end namespace runtime::trace

#endif /*__RUNTIME_TRANSPORT_TRACE_COMMON__*/
