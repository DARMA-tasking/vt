
#if ! defined __RUNTIME_TRANSPORT_TRACE_LOG__
#define __RUNTIME_TRANSPORT_TRACE_LOG__

#include "common.h"
#include "trace_common.h"

#include <cstdint>
#include <vector>
#include <memory>

namespace runtime { namespace trace {

struct Log {
  using log_ptr_t = std::shared_ptr<Log>;
  using log_container_t = std::vector<log_ptr_t>;
  using trace_type_t = TraceType;

  double time = 0.0;
  trace_event_id_t event = no_trace_event;
  trace_type_t type = trace_type_t::InvalidTraceType;
  trace_log_id_t log_id = no_log_id;
  node_t node = uninitialized_destination;

  log_ptr_t begin = nullptr, end = nullptr;

  Log(
    double const& in_time, /*trace_log_id_t const& in_log_id,*/
    trace_event_id_t const& in_event, TraceType const& in_type
  ) : time(in_time), /*log_id(in_log_id),*/ event(in_event), type(in_type)
  { }

  void
  add_dep(log_ptr_t const& log) {
    deps.push_back(log);
  }

  log_container_t deps;
};

}} //end namespace runtime::trace

#endif /*__RUNTIME_TRANSPORT_TRACE_LOG__*/
