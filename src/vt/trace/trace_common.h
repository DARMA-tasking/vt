
#if !defined INCLUDED_TRACE_TRACE_COMMON_H
#define INCLUDED_TRACE_TRACE_COMMON_H

#include "vt/config.h"

#include <cstdint>
#include <functional>
#include <string>

namespace vt { namespace trace {

static constexpr uint32_t const trace_flush_size = 100000;

using TraceEntryIDType = std::hash<std::string>::result_type;
using TraceEventIDType = uint32_t;
using TraceMsgLenType = size_t;

static constexpr TraceEntryIDType const no_trace_entry_id = -1;
static constexpr TraceEventIDType const no_trace_event = 0;
static constexpr NodeType const designated_root_node = 0;
static constexpr int64_t const trace_reserve_count = 1048576;

static constexpr BitCountType const trace_event_num_bits = 32;

}} //end namespace vt::trace

#endif /*INCLUDED_TRACE_TRACE_COMMON_H*/
