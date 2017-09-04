
#if ! defined __RUNTIME_TRANSPORT_TRACE__
#define __RUNTIME_TRANSPORT_TRACE__

#include "common.h"
#include "context.h"

#include "trace_common.h"
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

namespace runtime { namespace trace {

struct Trace {
  using log_t = Log;
  using trace_type_t = TraceType;
  using trace_cont_t = TraceContainers<void>;
  using time_int_t = int64_t;
  using log_ptr_t = std::shared_ptr<log_t>;
  using trace_container_t = std::vector<log_ptr_t>;
  using trace_atack_t = std::stack<log_ptr_t>;

  Trace(std::string const& in_prog_name, std::string const& in_trace_name)
    : prog_name(in_prog_name), trace_name(in_trace_name), start_time(MPI_Wtime())
  {
    traces.reserve(trace_reserve_count);
  }

  Trace() {
    traces.reserve(trace_reserve_count);
  }

  friend struct Log;

  void
  setup_names(
    std::string const& in_prog_name, std::string const& in_trace_name
  ) {
    prog_name = in_prog_name;
    trace_name = in_trace_name;
    start_time = MPI_Wtime();
  }

  virtual ~Trace() {
    write_traces_file();
  }

  static trace_event_id_t
  register_event_hashed(
    std::string const& event_type_name, std::string const& event_name
  ) {
    // must use this old-style of print because context may not be initialized
    #if backend_check_enabled(trace_enabled) && backend_check_enabled(trace)
    printf(
      "register_event_hashed: event_type_name=%s, event_name=%s\n",
      event_type_name.c_str(), event_name.c_str()
    );
    #endif

    trace_event_id_t event_type_seq = no_trace_event;
    event_type_t new_event_type(event_type_name);

    auto type_iter = trace_cont_t::event_type_container.find(
      new_event_type.get_event_id()
    );

    if (type_iter == trace_cont_t::event_type_container.end()) {
      event_type_seq = trace_cont_t::event_type_container.size();
      new_event_type.set_event_seq(event_type_seq);

      trace_cont_t::event_type_container.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(new_event_type.get_event_id()),
        std::forward_as_tuple(new_event_type)
      );
    } else {
      event_type_seq = type_iter->second.get_event_seq();
    }

    trace_event_id_t event_seq = no_trace_event;
    event_t new_event(event_name, new_event_type.get_event_id());

    new_event.set_event_type_seq(event_type_seq);

    auto event_iter = trace_cont_t::event_container.find(
      new_event.get_event_id()
    );

    if (event_iter == trace_cont_t::event_container.end()) {
      event_seq = trace_cont_t::event_container.size();
      new_event.set_event_seq(event_seq);

      trace_cont_t::event_container.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(new_event.get_event_id()),
        std::forward_as_tuple(new_event)
      );
    } else {
      event_seq = event_iter->second.get_event_seq();
    }

    return new_event.get_event_id();
  }

  log_ptr_t
  event_start(trace_event_id_t const& event, double const& time = MPI_Wtime()) {
    auto const& type = trace_type_t::TraceGroupedBegin;
    log_ptr_t log = std::make_shared<log_t>(time, event, type);

    debug_print(
      trace, node,
      "event_start: event=%lu, time=%f\n", event, time
    );

    log->node = the_context->get_node();

    log_event(log);

    return log;
  }

  log_ptr_t
  event_stop(trace_event_id_t const& event, double const& time = MPI_Wtime()) {
    auto const& type = trace_type_t::TraceGroupedEnd;
    log_ptr_t log = std::make_shared<log_t>(time, event, type);

    debug_print(
      trace, node,
      "event_stop: event=%lu, time=%f\n", event, time
    );

    log->node = the_context->get_node();

    log_event(log);

    return log;
  }

  log_ptr_t
  create_new_dep(trace_event_id_t const& event, double const& time = MPI_Wtime()) {
    auto const& type = trace_type_t::TraceDepCreate;
    log_ptr_t log = std::make_shared<log_t>(time, event, type);

    log->node = the_context->get_node();

    return log;
  }

  void
  register_new_dep_current(log_ptr_t const& new_dep) {
    debug_print(
      trace, node,
      "register_new_dep_current: open_events.size=%ld\n", open_events.size()
    );

    if (not open_events.empty()) {
      //auto const& top = open_events.top();
      //top->add_dep(new_dep);
      log_event(new_dep);
    }
  }

  trace_log_id_t
  log_event(log_ptr_t log) {
    if (not enabled) {
      return 0;
    }

    auto grouped_begin = [&]() -> trace_log_id_t {
      if (not open_events.empty()) {
        traces.push_back(
          std::make_shared<log_t>(
            log->time, open_events.top()->event, trace_type_t::TraceGroupedEnd
          )
        );
      }

      // push on open stack.
      open_events.push(log);
      traces.push_back(log);

      trace_log_id_t const& id = traces.size() - 1;

      log->log_id = id;

      return id;
    };

    auto grouped_end = [&]() -> trace_log_id_t {
      assert(
        not open_events.empty() and "Stack should be empty"
      );

      assert(
        open_events.top()->event == log->event and
        open_events.top()->type == trace_type_t::TraceGroupedBegin and
        "Top event should be correct type and event"
      );

      // match event with the one that this ends
      log->log_id = open_events.top()->log_id;

      // set up begin/end links
      open_events.top()->end = log;
      log->begin = open_events.top();
      open_events.pop();

      traces.push_back(log);

      if (not open_events.empty()) {
        traces.push_back(
          std::make_shared<log_t>(
            log->time, open_events.top()->event, trace_type_t::TraceGroupedBegin
          )
        );
      }

      return log->log_id;
    };

    auto dep_create = [&]() -> trace_log_id_t {
      traces.push_back(log);

      trace_log_id_t const& id = traces.size() - 1;
      log->log_id = id;

      return id;
    };

    switch (log->type) {
    case trace_type_t::TraceGroupedBegin:
      return grouped_begin();
      break;
    case trace_type_t::TraceGroupedEnd:
      return grouped_end();
      break;
    case trace_type_t::TraceDepCreate:
      return dep_create();
      break;
    default:
      assert(0 and "Not implemented");
      return 0;
      break;
    }
  }

  void
  enabled_tracing() {
    enabled = true;
  };

  void
  disable_tracing() {
    enabled = false;
  };

  void
  write_traces_file() {
    auto const& node = the_context->get_node();
    auto const& num_nodes = the_context->get_num_nodes();

    debug_print(
      trace, node,
      "write_traces_file: traces.size=%ld, "
      "event_type_container.size=%ld, event_container.size=%ld\n",
      traces.size(),
      trace_cont_t::event_type_container.size(),
      trace_cont_t::event_container.size()
    );

    std::ofstream file;
    file.open(trace_name);
    output_header(node, start_time, file);
    write_log_file(file, traces);
    output_footer(node, start_time, file);
    file.flush();
    file.close();

    if (node == designated_root_node) {
      std::ofstream file;
      file.open(prog_name + ".sts");
      output_control_file(file);
      file.flush();
      file.close();
    }
  }

  void
  write_log_file(std::ofstream& file, trace_container_t const& traces) {
    for (auto&& log : traces) {
      auto const& converted_time = time_to_int(log->time - start_time);

      auto const& type = static_cast<
        std::underlying_type<decltype(log->type)>::type
      >(log->type);

      auto event_iter = trace_cont_t::event_container.find(log->event);

      assert(
        event_iter != trace_cont_t::event_container.end() and
        "Event must exist that was logged"
      );

      auto const& event_seq_id = event_iter->second.get_event_seq();

      switch (log->type) {
      case trace_type_t::TraceGroupedBegin:
        file << type << " 0 "
             << event_seq_id << " "
             << converted_time << " "
             << log->log_id << " "
             << log->node << " "
             << "0 0 0 0 0 0 0\n";
        break;
      case trace_type_t::TraceGroupedEnd:
        file << type << " 0 "
             << event_seq_id << " "
             << converted_time << " "
             << log->log_id << " "
             << log->node << " "
             << "0 0 0 0 0 0 0 0\n";
        break;
      case trace_type_t::TraceDepCreate:
        file << type << " 0 "
             << event_seq_id << " "
             << converted_time << " "
             << log->log_id << " "
             << log->node << " "
             << "0 0 0 0 0 0 0 0\n";
        break;
      default:
        assert(0);
      }

      // recursive call to unfold trace structure
      if (log->deps.size() > 0) {
        write_log_file(file, log->deps);
      }
    }

    traces.empty();
  }

  static void
  output_control_file(std::ofstream& file) {
    auto const& node = the_context->get_node();
    auto const& num_nodes = the_context->get_num_nodes();

    file << "PROJECTIONS_ID\n"
         << "VERSION 7.0\n"
         << "TOTAL_PHASES 1\n"
         << "MACHINE unknown\n"
         << "PROCESSORS " << num_nodes << "\n"
         << "TOTAL_CHARES " << trace_cont_t::event_type_container.size() << "\n"
         << "TOTAL_EPS " << trace_cont_t::event_container.size() << "\n"
         << "TOTAL_MSGS 0\n"
         << "TOTAL_PSEUDOS 0\n"
         << "TOTAL_EVENTS 0"
         << std::endl;

    container_event_sorted_t sorted_event;
    container_event_type_sorted_t sorted_event_type;

    for (auto&& elem : trace_cont_t::event_container) {
      sorted_event.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(&elem.second),
        std::forward_as_tuple(true)
      );
    }

    for (auto&& elem : trace_cont_t::event_type_container) {
      sorted_event_type.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(&elem.second),
        std::forward_as_tuple(true)
      );
    }

    for (auto&& event : sorted_event_type) {
      auto const& name = event.first->get_event_name();
      auto const& id = event.first->get_event_seq();

      auto const& out_name = std::string("::" + name);

      file << "CHARE "
           << id << " "
           << out_name << " "
           << std::endl;
    }

    for (auto&& event : sorted_event) {
      auto const& name = event.first->get_event_name();
      auto const& type = event.first->get_event_type_seq();
      auto const& id = event.first->get_event_seq();

      file << "ENTRY CHARE "
           << id << " "
           << name << " "
           << type << " "
           << id << " "
           << std::endl;
    }

    file << "TOTAL_FUNCTIONS 0\n"
         << "END\n"
         << std::endl;
  }

  static void
  output_header(node_t const& node, double const& start, std::ofstream& file) {
    // Output header for projections file
    file << "PROJECTIONS-RECORD 0" << std::endl;
    // '6' means COMPUTATION_BEGIN to Projections: this starts a trace
    file << "6 " << 0 << std::endl;
  }

  static void
  output_footer(node_t const& node, double const& start, std::ofstream& file) {
    // Output footer for projections file, '7' means COMPUTATION_END to
    // Projections
    file << "7 " << time_to_int(MPI_Wtime() - start) << std::endl;
  }

  static time_int_t
  time_to_int(double const& time) {
    return static_cast<time_int_t>(time * 1e6);
  }

private:
  trace_container_t traces;

  std::stack<log_ptr_t> open_events;

  bool enabled = true;

  std::string prog_name, trace_name;

  double start_time = 0.0;
};

}} //end namespace runtime::trace

namespace runtime {

backend_enable_if(
  trace_enabled,
  extern std::unique_ptr<trace::Trace> the_trace;
);

}

#endif /*__RUNTIME_TRANSPORT_TRACE__*/
