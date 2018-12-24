
#if !defined INCLUDED_TRACE_TRACE_H
#define INCLUDED_TRACE_TRACE_H

#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/configs/arguments/args.h"
#include "vt/trace/trace_common.h"
#include "vt/trace/trace_registry.h"
#include "vt/trace/trace_constants.h"
#include "vt/trace/trace_event.h"
#include "vt/trace/trace_containers.h"
#include "vt/trace/trace_log.h"

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
  using LogType             = Log;
  using TraceConstantsType  = eTraceConstants;
  using TraceContainersType = TraceContainers<void>;
  using TimeIntegerType     = int64_t;
  using LogPtrType          = LogType*;
  using TraceContainerType  = std::vector<LogPtrType>;
  using TraceStackType      = std::stack<LogPtrType>;
  using ArgType             = vt::arguments::ArgConfig;

  Trace();
  Trace(std::string const& in_prog_name, std::string const& in_trace_name);

  virtual ~Trace();

  friend struct Log;

  std::string getTraceName() const { return full_trace_name; }
  std::string getSTSName()   const { return full_sts_name;   }
  std::string getDirectory() const { return full_dir_name;   }

  void initialize();
  void setupNames(
    std::string const& in_prog_name, std::string const& in_trace_name,
    std::string const& in_dir_name = ""
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
  bool checkEnabled();

  void writeTracesFile();
  void cleanupTracesFile();
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
  std::string dir_name_       = "";
  std::string prog_name_      = "";
  std::string trace_name_     = "";
  bool enabled_               = true;
  bool idle_begun_            = false;
  bool use_directory_         = false;
  double start_time_          = 0.0;
  std::string full_trace_name = "";
  std::string full_sts_name   = "";
  std::string full_dir_name   = "";
  gzFile log_file;
  bool file_is_open = false;
  bool wrote_sts_file = false;
  int64_t cur = 0;
};

}} //end namespace vt::trace

namespace vt {

backend_enable_if(
  trace_enabled,
  extern trace::Trace* theTrace();
);

}

#endif /*INCLUDED_TRACE_TRACE_H*/
