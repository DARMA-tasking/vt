
#if !defined INCLUDED_TRACE_TRACE_H
#define INCLUDED_TRACE_TRACE_H

#include "config.h"
#include "context/context.h"

#include "trace/trace_common.h"
#include "trace/trace_registry.h"
#include "trace/trace_constants.h"
#include "trace/trace_event.h"
#include "trace/trace_containers.h"
#include "trace/trace_log.h"

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

namespace vt { namespace trace {

struct Trace {
  using LogType = Log;
  using TraceConstantsType = eTraceConstants;
  using TraceContainersType = TraceContainers<void>;
  using TimeIntegerType = int64_t;
  using LogPtrType = LogType*;
  using TraceContainerType = std::vector<LogPtrType>;
  using TraceStackType = std::stack<LogPtrType>;

  Trace();
  Trace(std::string const& in_prog_name, std::string const& in_trace_name);

  virtual ~Trace();

  friend struct Log;

  void initialize();
  void setupNames(
    std::string const& in_prog_name, std::string const& in_trace_name
  );

  void beginProcessing(
    TraceEntryIDType const& ep, TraceMsgLenType const& len,
    TraceEventIDType const& event, NodeType const& from_node,
    double const& time = getCurrentTime(),
    uint64_t const idx = 0
  );
  void endProcessing(
    TraceEntryIDType const& ep, TraceMsgLenType const& len,
    TraceEventIDType const& event, NodeType const& from_node,
    double const& time = getCurrentTime(),
    uint64_t const idx = 0
  );

  void beginIdle(double const& time = getCurrentTime());
  void endIdle(double const& time = getCurrentTime());

  TraceEventIDType messageCreation(
    TraceEntryIDType const& ep, TraceMsgLenType const& len,
    double const& time = getCurrentTime()
  );
  TraceEventIDType messageCreationBcast(
    TraceEntryIDType const& ep, TraceMsgLenType const& len,
    double const& time = getCurrentTime()
  );
  TraceEventIDType messageRecv(
    TraceEntryIDType const& ep, TraceMsgLenType const& len,
    NodeType const& from_node, double const& time = getCurrentTime()
  );
  TraceEventIDType logEvent(LogPtrType log);

  void enableTracing();
  void disableTracing();

  void writeTracesFile();
  void writeLogFile(gzFile file, TraceContainerType const& traces);
  bool inIdleEvent() const;

  static double getCurrentTime();
  static void outputControlFile(std::ofstream& file);
  static TimeIntegerType timeToInt(double const& time);
  static void traceBeginIdleTrigger();
  static void outputHeader(
    NodeType const& node, double const& start, gzFile file
  );
  static void outputFooter(
    NodeType const& node, double const& start, gzFile file
  );

private:
  TraceContainerType traces_;
  TraceStackType open_events_;
  TraceEventIDType cur_event_ = 1;
  std::string dir_name_;
  std::string prog_name_, trace_name_;

  bool enabled_ = true, idle_begun_ = false;

  double start_time_ = 0.0;
};

}} //end namespace vt::trace

namespace vt {

backend_enable_if(
  trace_enabled,
  extern trace::Trace* theTrace();
);

}

#endif /*INCLUDED_TRACE_TRACE_H*/
