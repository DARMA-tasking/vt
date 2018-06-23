
#include "config.h"
#include "trace/trace.h"
#include "timing/timing.h"
#include "scheduler/scheduler.h"
#include "utils/demangle/demangle.h"

#include <zlib.h>
#include <sys/stat.h>
#include <unistd.h>

#define trace_use_dir 1

namespace vt { namespace trace {

Trace::Trace(std::string const& in_prog_name, std::string const& in_trace_name)
  : prog_name_(in_prog_name), trace_name_(in_trace_name),
    start_time_(getCurrentTime())
{
  initialize();
}

Trace::Trace() {
  initialize();
}

/*static*/ void Trace::traceBeginIdleTrigger() {
  backend_enable_if(
    trace_enabled, {
      if (not theTrace()->inIdleEvent()) {
        theTrace()->beginIdle();
      }
    }
  );
}

void Trace::initialize() {
  traces_.reserve(trace_reserve_count);

  theSched()->registerTrigger(
    sched::SchedulerEvent::BeginIdle, traceBeginIdleTrigger
  );
}

bool Trace::inIdleEvent() const {
  return idle_begun_;
}

void Trace::setupNames(
  std::string const& in_prog_name, std::string const& in_trace_name,
  std::string const& in_dir_name
) {
  prog_name_ = in_prog_name;
  trace_name_ = in_trace_name;
  dir_name_ = in_dir_name;
  start_time_ = getCurrentTime();

  #if trace_use_dir
  if (theContext()->getNode() == 0) {
    if (dir_name_ != "") {
      bool made_dir = true, have_cur_directory = true;
      char cur_dir[1024];
      if (getcwd(cur_dir, sizeof(cur_dir)) == nullptr) {
        have_cur_directory = false;
      }
      if (have_cur_directory) {
        auto full_dir_name = std::string(cur_dir) + "/" + dir_name_;
        auto const dir_ret = mkdir(full_dir_name.c_str(), S_IRWXU);
        if (dir_ret == -1) {
          made_dir = false;
        }
        if (made_dir) {
          use_directory_ = true;
        }
      }
    }
  }
  #endif
}

/*virtual*/ Trace::~Trace() {
  writeTracesFile();
}

void Trace::beginProcessing(
  TraceEntryIDType const& ep, TraceMsgLenType const& len,
  TraceEventIDType const& event, NodeType const& from_node, double const& time,
  uint64_t const idx
) {
  auto const& type = TraceConstantsType::BeginProcessing;
  LogPtrType log = new LogType(time, ep, type);

  debug_print(
    trace, node,
    "event_start: ep={}, event={}, time={}\n", ep, event, time
  );

  log->node = from_node;
  log->msg_len = len;
  log->event = event;
  log->idx = idx;

  logEvent(log);
}

void Trace::endProcessing(
  TraceEntryIDType const& ep, TraceMsgLenType const& len,
  TraceEventIDType const& event, NodeType const& from_node, double const& time,
  uint64_t const idx
) {
  auto const& type = TraceConstantsType::EndProcessing;
  LogPtrType log = new LogType(time, ep, type);

  debug_print(
    trace, node,
    "event_stop: ep={}, event={}, time={}\n", ep, event, time
  );

  log->node = from_node;
  log->msg_len = len;
  log->event = event;
  log->idx = idx;

  logEvent(log);
}

void Trace::beginIdle(double const& time) {
  auto const& type = TraceConstantsType::BeginIdle;
  LogPtrType log = new LogType(time, no_trace_entry_id, type);

  debug_print(
    trace, node, "begin_idle: time=%f\n", time
  );

  log->node = theContext()->getNode();

  logEvent(log);

  idle_begun_ = true;
}

void Trace::endIdle(double const& time) {
  auto const& type = TraceConstantsType::EndIdle;
  LogPtrType log = new LogType(time, no_trace_entry_id, type);

  debug_print(
    trace, node, "end_idle: time=%f\n", time
  );

  log->node = theContext()->getNode();

  logEvent(log);

  idle_begun_ = false;
}

TraceEventIDType Trace::messageCreation(
  TraceEntryIDType const& ep, TraceMsgLenType const& len, double const& time
) {
  auto const& type = TraceConstantsType::Creation;
  LogPtrType log = new LogType(time, ep, type);

  log->node = theContext()->getNode();
  log->msg_len = len;

  return logEvent(log);
}

TraceEventIDType Trace::messageCreationBcast(
  TraceEntryIDType const& ep, TraceMsgLenType const& len, double const& time
) {
  auto const& type = TraceConstantsType::CreationBcast;
  LogPtrType log = new LogType(time, ep, type);

  log->node = theContext()->getNode();
  log->msg_len = len;

  return logEvent(log);
}

TraceEventIDType Trace::messageRecv(
  TraceEntryIDType const& ep, TraceMsgLenType const& len,
  NodeType const& from_node, double const& time
) {
  auto const& type = TraceConstantsType::MessageRecv;
  LogPtrType log = new LogType(time, ep, type);

  log->node = from_node;

  return logEvent(log);
}

TraceEventIDType Trace::logEvent(LogPtrType log) {
  if (not enabled_) {
    return 0;
  }

  // close any idle event as soon as we encounter any other type of event
  if (idle_begun_ and
      log->type != TraceConstantsType::BeginIdle and
      log->type != TraceConstantsType::EndIdle) {
    endIdle();
  }

  auto grouped_begin = [&]() -> TraceEventIDType {
    if (not open_events_.empty()) {
      traces_.push_back(
        new LogType(
          log->time, open_events_.top()->ep, TraceConstantsType::EndProcessing
        )
      );
    }

    // push on open stack.
    open_events_.push(log);
    traces_.push_back(log);

    return log->event;
  };

  auto grouped_end = [&]() -> TraceEventIDType {
    assert(
      not open_events_.empty() and "Stack should be empty"
    );

    assert(
      open_events_.top()->ep == log->ep and
      open_events_.top()->type == TraceConstantsType::BeginProcessing and
      "Top event should be correct type and event"
    );

    // match event with the one that this ends
    log->event = open_events_.top()->event;

    // set up begin/end links
    open_events_.pop();

    traces_.push_back(log);

    if (not open_events_.empty()) {
      traces_.push_back(
        new LogType(
          log->time, open_events_.top()->ep, TraceConstantsType::BeginProcessing
        )
      );
    }

    return log->event;
  };

  auto basic_new_event_create = [&]() -> TraceEventIDType {
    traces_.push_back(log);

    log->event = cur_event_++;

    return log->event;
  };

  auto basic_no_event_create = [&]() -> TraceEventIDType {
    traces_.push_back(log);

    log->event = no_trace_event;

    return log->event;
  };

  switch (log->type) {
  case TraceConstantsType::BeginProcessing:
    return grouped_begin();
    break;
  case TraceConstantsType::EndProcessing:
    return grouped_end();
    break;
  case TraceConstantsType::Creation:
  case TraceConstantsType::CreationBcast:
  case TraceConstantsType::MessageRecv:
    return basic_new_event_create();
    break;
  case TraceConstantsType::BeginIdle:
  case TraceConstantsType::EndIdle:
    return basic_no_event_create();
    break;
  default:
    assert(0 and "Not implemented");
    return 0;
    break;
  }
}

void Trace::enableTracing() {
  enabled_ = true;
};

void Trace::disableTracing() {
  enabled_ = false;
};

void Trace::writeTracesFile() {
  auto const& node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  debug_print(
    trace, node,
    "write_traces_file: traces.size={}, "
    "event_type_container.size={}, event_container.size={}\n",
    traces_.size(),
    TraceContainersType::event_type_container.size(),
    TraceContainersType::event_container.size()
  );

  #if trace_use_dir
  auto const tc = util::demangle::DemanglerUtils::splitString(trace_name_, '/');
  auto const pc = util::demangle::DemanglerUtils::splitString(prog_name_, '/');
  auto const trace_name = tc[tc.size()-1];
  auto const prog_name = tc[tc.size()-1];
  #endif

  std::string full_trace_name = trace_name_;
  std::string full_sts_name = prog_name_ + ".sts";

  #if trace_use_dir
  if (dir_name_ != "") {
    bool made_dir = true, have_cur_directory = true;
    char cur_dir[1024];
    if (getcwd(cur_dir, sizeof(cur_dir)) == nullptr) {
      have_cur_directory = false;
    }

    if (have_cur_directory) {
      auto full_dir_name = std::string(cur_dir) + "/" + dir_name_;
      struct stat info;
      if (stat(full_dir_name.c_str(), &info) != 0) {
        use_directory_ = false;
      } else {
        use_directory_ = true;
      }

      if (use_directory_) {
        full_trace_name = full_dir_name + "/" + trace_name;
        full_sts_name = full_dir_name + "/" + prog_name + ".sts";
      }
    }
  }
  #endif

  gzFile file = gzopen(full_trace_name.c_str(), "wb");
  outputHeader(node, start_time_, file);
  writeLogFile(file, traces_);
  outputFooter(node, start_time_, file);
  gzclose(file);

  if (node == designated_root_node) {
    std::ofstream file;
    file.open(full_sts_name);
    outputControlFile(file);
    file.flush();
    file.close();
  }
}

void Trace::writeLogFile(gzFile file, TraceContainerType const& traces) {
  for (auto&& log : traces) {
    auto const& converted_time = timeToInt(log->time - start_time_);

    auto const& type = static_cast<
      std::underlying_type<decltype(log->type)>::type
        >(log->type);

    auto event_iter = TraceContainersType::getEventContainer().find(log->ep);

    assert(
      log->ep == no_trace_entry_id or
      event_iter != TraceContainersType::getEventContainer().end() and
      "Event must exist that was logged"
    );

    auto const& event_seq_id = log->ep == no_trace_entry_id ?
      no_trace_entry_id : event_iter->second.theEventSeq();

    auto const& num_nodes = theContext()->getNumNodes();

    switch (log->type) {
    case TraceConstantsType::BeginProcessing:
      gzprintf(
        file,
        "%d %d %lu %lld %d %d %d 0 %d 0 0 0 0\n",
        type,
        eTraceEnvelopeTypes::ForChareMsg,
        event_seq_id,
        converted_time,
        log->event,
        log->node,
        log->msg_len,
        log->idx
      );
      break;
    case TraceConstantsType::EndProcessing:
      gzprintf(
        file,
        "%d %d %lu %lld %d %d %d 0 %d 0 0 0 0\n",
        type,
        eTraceEnvelopeTypes::ForChareMsg,
        event_seq_id,
        converted_time,
        log->event,
        log->node,
        log->msg_len,
        log->idx
      );
      break;
    case TraceConstantsType::BeginIdle:
      gzprintf(
        file,
        "%d %lld %d\n",
        type,
        converted_time,
        log->node
      );
      break;
    case TraceConstantsType::EndIdle:
      gzprintf(
        file,
        "%d %lld %d\n",
        type,
        converted_time,
        log->node
      );
      break;
    case TraceConstantsType::CreationBcast:
      gzprintf(
        file,
        "%d %d %lu %lld %d %d %d %d %d\n",
        type,
        eTraceEnvelopeTypes::ForChareMsg,
        event_seq_id,
        converted_time,
        log->event,
        log->node,
        log->msg_len,
        0,
        num_nodes
      );
      break;
    case TraceConstantsType::Creation:
      gzprintf(
        file,
        "%d %d %lu %lld %d %d %d 0\n",
        type,
        eTraceEnvelopeTypes::ForChareMsg,
        event_seq_id,
        converted_time,
        log->event,
        log->node,
        log->msg_len
      );
      break;
    case TraceConstantsType::MessageRecv:
      assert(0);
      break;
    default:
      assert(0);
    }

    delete log;
  }

  traces.empty();
}

/*static*/ double Trace::getCurrentTime() {
  return timing::Timing::getCurrentTime();
}

/*static*/ void Trace::outputControlFile(std::ofstream& file) {
  auto const& node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  auto const& num_event_types =
    TraceContainersType::getEventTypeContainer().size();
  auto const& num_events = TraceContainersType::getEventContainer().size();

  file << "PROJECTIONS_ID\n"
       << "VERSION 7.0\n"
       << "TOTAL_PHASES 1\n"
       << "MACHINE vt\n"
       << "PROCESSORS " << num_nodes << "\n"
       << "TOTAL_CHARES " << num_event_types << "\n"
       << "TOTAL_EPS " << num_events << "\n"
       << "TOTAL_MSGS 1\n"
       << "TOTAL_PSEUDOS 0\n"
       << "TOTAL_EVENTS 0"
       << std::endl;

  ContainerEventSortedType sorted_event;
  ContainerEventTypeSortedType sorted_event_type;

  for (auto&& elem : TraceContainersType::getEventContainer()) {
    sorted_event.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(&elem.second),
      std::forward_as_tuple(true)
    );
  }

  for (auto&& elem : TraceContainersType::getEventTypeContainer()) {
    sorted_event_type.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(&elem.second),
      std::forward_as_tuple(true)
    );
  }

  for (auto&& event : sorted_event_type) {
    auto const& name = event.first->theEventName();
    auto const& id = event.first->theEventSeq();

    auto const& out_name = std::string("::" + name);

    file << "CHARE "
         << id << " "
         << out_name << " "
         << std::endl;
  }

  for (auto&& event : sorted_event) {
    auto const& name = event.first->theEventName();
    auto const& type = event.first->theEventTypeSeq();
    auto const& id = event.first->theEventSeq();

    file << "ENTRY CHARE "
         << id << " "
         << name << " "
         << type << " "
         << 0 << " "
         << std::endl;
  }

  file << "MESSAGE 0 0\n"
       << "TOTAL_STATS 0\n"
       << "TOTAL_FUNCTIONS 0\n"
       << "END\n"
       << std::endl;
}

/*static*/ void Trace::outputHeader(
  NodeType const& node, double const& start, gzFile file
) {
  // Output header for projections file
  gzprintf(file, "PROJECTIONS-RECORD 0\n");
  // '6' means COMPUTATION_BEGIN to Projections: this starts a trace
  gzprintf(file, "6 0\n");
}

/*static*/ void Trace::outputFooter(
  NodeType const& node, double const& start, gzFile file
) {
  // Output footer for projections file, '7' means COMPUTATION_END to
  // Projections
  gzprintf(file, "7 %lld\n", timeToInt(getCurrentTime() - start));
}

/*static*/ Trace::TimeIntegerType Trace::timeToInt(double const& time) {
  return static_cast<TimeIntegerType>(time * 1e6);
}

}} //end namespace vt::trace
