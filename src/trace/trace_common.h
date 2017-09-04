
#if ! defined __RUNTIME_TRANSPORT_TRACE_COMMON__
#define __RUNTIME_TRANSPORT_TRACE_COMMON__

#include "common.h"

#include <cstdint>
#include <functional>
#include <string>

namespace runtime { namespace trace {

static constexpr uint32_t const trace_flush_size = 100000;

using trace_event_id_t = std::hash<std::string>::result_type;
using trace_log_id_t = int64_t;

static constexpr trace_event_id_t const no_trace_event = -1;
static constexpr trace_log_id_t const no_log_id = -1;
static constexpr node_t const designated_root_node = 0;
static constexpr int64_t const trace_reserve_count = 1048576;

}} //end namespace runtime::trace

#endif /*__RUNTIME_TRANSPORT_TRACE_COMMON__*/
