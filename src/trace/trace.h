
#if ! defined __RUNTIME_TRANSPORT_TRACE__
#define __RUNTIME_TRANSPORT_TRACE__

#include "config.h"
#include "context/context.h"

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
    double const& time = getCurrentTime()
  );
  void endProcessing(
    TraceEntryIDType const& ep, TraceMsgLenType const& len,
    TraceEventIDType const& event, NodeType const& from_node,
    double const& time = getCurrentTime()
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

  std::string prog_name_, trace_name_;

  bool enabled_ = true, idle_begun_ = false;

  double start_time_ = 0.0;
};

}} //end namespace vt::trace

namespace vt {

backend_enable_if(
  trace_enabled,
  extern std::unique_ptr<trace::Trace> theTrace;
);

}

#endif /*__RUNTIME_TRANSPORT_TRACE__*/
