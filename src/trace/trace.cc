
#include "config.h"
#include "trace.h"
#include "timing/timing.h"
#include "scheduler/scheduler.h"

#include <zlib.h>

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
  std::string const& in_prog_name, std::string const& in_trace_name
) {
  prog_name_ = in_prog_name;
  trace_name_ = in_trace_name;
  start_time_ = getCurrentTime();
}

/*virtual*/ Trace::~Trace() {
  writeTracesFile();
}

void Trace::beginProcessing(
  TraceEntryIDType const& ep, TraceMsgLenType const& len,
  TraceEventIDType const& event, NodeType const& from_node, double const& time
) {
  auto const& type = TraceConstantsType::BeginProcessing;
  LogPtrType log = new LogType(time, ep, type);

  debug_print(
    trace, node,
    "event_start: ep=%lu, event=%d, time=%f\n", ep, event, time
  );

  log->node = from_node;
  log->msg_len = len;
  log->event = event;

  logEvent(log);
}

void Trace::endProcessing(
  TraceEntryIDType const& ep, TraceMsgLenType const& len,
  TraceEventIDType const& event, NodeType const& from_node, double const& time
) {
  auto const& type = TraceConstantsType::EndProcessing;
  LogPtrType log = new LogType(time, ep, type);

  debug_print(
    trace, node,
    "event_stop: ep=%lu, event=%d, time=%f\n", ep, event, time
  );

  log->node = from_node;
  log->msg_len = len;
  log->event = event;

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
    "write_traces_file: traces.size=%ld, "
    "event_type_container.size=%ld, event_container.size=%ld\n",
    traces_.size(),
    TraceContainersType::event_type_container.size(),
    TraceContainersType::event_container.size()
  );

  gzFile file = gzopen(trace_name_.c_str(), "wb");
  outputHeader(node, start_time_, file);
  writeLogFile(file, traces_);
  outputFooter(node, start_time_, file);
  gzclose(file);

  if (node == designated_root_node) {
    std::ofstream file;
    file.open(prog_name_ + ".sts");
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

    auto event_iter = TraceContainersType::event_container.find(log->ep);

    assert(
      log->ep == no_trace_entry_id or
      event_iter != TraceContainersType::event_container.end() and
      "Event must exist that was logged"
    );

    auto const& event_seq_id = log->ep == no_trace_entry_id ?
      no_trace_entry_id : event_iter->second.theEventSeq();

    auto const& num_nodes = theContext()->getNumNodes();

    switch (log->type) {
    case TraceConstantsType::BeginProcessing:
      gzprintf(
        file,
        "%d %d %lu %lld %d %d 0 0 0 0 0 0 0\n",
        type,
        eTraceEnvelopeTypes::ForChareMsg,
        event_seq_id,
        converted_time,
        log->event,
        log->node
      );
      break;
    case TraceConstantsType::EndProcessing:
      gzprintf(
        file,
        "%d %d %lu %lld %d %d 0 0 0 0 0 0 0\n",
        type,
        eTraceEnvelopeTypes::ForChareMsg,
        event_seq_id,
        converted_time,
        log->event,
        log->node
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

  auto const& num_event_types = TraceContainersType::event_type_container.size();
  auto const& num_events = TraceContainersType::event_container.size();

  file << "PROJECTIONS_ID\n"
       << "VERSION 7.0\n"
       << "TOTAL_PHASES 1\n"
       << "MACHINE unknown\n"
       << "PROCESSORS " << num_nodes << "\n"
       << "TOTAL_CHARES " << num_event_types << "\n"
       << "TOTAL_EPS " << num_events << "\n"
       << "TOTAL_MSGS 0\n"
       << "TOTAL_PSEUDOS 0\n"
       << "TOTAL_EVENTS 0"
       << std::endl;

  ContainerEventSortedType sorted_event;
  ContainerEventTypeSortedType sorted_event_type;

  for (auto&& elem : TraceContainersType::event_container) {
    sorted_event.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(&elem.second),
      std::forward_as_tuple(true)
    );
  }

  for (auto&& elem : TraceContainersType::event_type_container) {
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
         << id << " "
         << std::endl;
  }

  file << "TOTAL_FUNCTIONS 0\n"
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
