
#if ! defined __RUNTIME_TRANSPORT_TRACE__
#define __RUNTIME_TRANSPORT_TRACE__

#include "common.h"
#include "context.h"

#include "trace_common.h"
#include "trace_registry.h"
#include "trace_constants.h"
#include "trace_event.h"
#include "trace_containers.h"
#include "trace_log.h"

#include <cstdint>
#include <cassert>
#include <unordered_map>
#include <stack>
#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <iostream>
#include <fstream>

#include <mpi.h>
#include <zlib.h>

namespace runtime { namespace trace {

struct Trace {
  using log_t = Log;
  using trace_type_t = TraceConstants;
  using trace_cont_t = TraceContainers<void>;
  using time_int_t = int64_t;
  using log_ptr_t = log_t*;
  using trace_container_t = std::vector<log_ptr_t>;
  using trace_stack_t = std::stack<log_ptr_t>;

  Trace();

  Trace(std::string const& in_prog_name, std::string const& in_trace_name);

  void
  initialize();

  friend struct Log;

  void
  setup_names(
    std::string const& in_prog_name, std::string const& in_trace_name
  );

  virtual ~Trace();

  void
  begin_processing(
    TraceEntryType const& ep, trace_msg_len_t const& len, trace_event_t const& event,
    NodeType const& from_node, double const& time = get_current_time()
  );

  void
  end_processing(
    TraceEntryType const& ep, trace_msg_len_t const& len, trace_event_t const& event,
    NodeType const& from_node, double const& time = get_current_time()
  );

  void
  begin_idle(double const& time = get_current_time());

  void
  end_idle(double const& time = get_current_time());

  trace_event_t
  message_creation(
    TraceEntryType const& ep, trace_msg_len_t const& len,
    double const& time = get_current_time()
  );

  trace_event_t
  message_creation_bcast(
    TraceEntryType const& ep, trace_msg_len_t const& len,
    double const& time = get_current_time()
  );

  trace_event_t
  message_recv(
    TraceEntryType const& ep, trace_msg_len_t const& len, NodeType const& from_node,
    double const& time = get_current_time()
  );

  trace_event_t
  log_event(log_ptr_t log);

  void
  enable_tracing();

  void
  disable_tracing();

  void
  write_traces_file();

  void
  write_log_file(gzFile file, trace_container_t const& traces);

  bool
  in_idle_event() const;

  static double
  get_current_time();

  static void
  output_control_file(std::ofstream& file);

  static void
  output_header(NodeType const& node, double const& start, gzFile file);

  static void
  output_footer(NodeType const& node, double const& start, gzFile file);

  static time_int_t
  time_to_int(double const& time);

  static void
  trace_begin_idle_trigger();

private:
  trace_container_t traces;

  trace_stack_t open_events;

  bool enabled = true;

  std::string prog_name, trace_name;

  double start_time = 0.0;

  trace_event_t cur_event = 1;

  bool idle_begun = false;
};

}} //end namespace runtime::trace

namespace runtime {

backend_enable_if(
  trace_enabled,
  extern std::unique_ptr<trace::Trace> the_trace;
);

}

#endif /*__RUNTIME_TRANSPORT_TRACE__*/
