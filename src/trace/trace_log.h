
#if ! defined __RUNTIME_TRANSPORT_TRACE_LOG__
#define __RUNTIME_TRANSPORT_TRACE_LOG__

#include "common.h"
#include "trace_common.h"
#include "trace_constants.h"

#include <cstdint>
#include <vector>
#include <memory>

namespace runtime { namespace trace {

struct Log {
  using log_ptr_t = std::shared_ptr<Log>;
  using log_container_t = std::vector<log_ptr_t>;
  using trace_type_t = TraceConstants;

  double time = 0.0;
  trace_ep_t ep = no_trace_ep;
  trace_type_t type = trace_type_t::InvalidTraceType;
  trace_event_t event = no_trace_event;
  trace_msg_len_t msg_len = 0;
  node_t node = uninitialized_destination;

  log_ptr_t begin = nullptr, end = nullptr;

  Log(
    double const& in_time, trace_ep_t const& in_ep,
    trace_type_t const& in_type, trace_msg_len_t const& in_msg_len = 0
  ) : time(in_time), ep(in_ep), type(in_type), msg_len(in_msg_len)
  { }

  void
  add_dep(log_ptr_t const& log) {
    deps.push_back(log);
  }

  log_container_t deps;
};

}} //end namespace runtime::trace

#endif /*__RUNTIME_TRANSPORT_TRACE_LOG__*/
