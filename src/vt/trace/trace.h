/*
//@HEADER
// *****************************************************************************
//
//                                   trace.h
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_TRACE_TRACE_H
#define INCLUDED_TRACE_TRACE_H

#include "vt/trace/trace_common.h"
#include "vt/trace/trace_containers.h"
#include "vt/trace/trace_log.h"
#include "vt/trace/trace_registry.h"
#include "vt/trace/trace_user_event.h"

#include "vt/timing/timing.h"

#include <cassert>
#include <cstdint>
#include <functional>
#include <iosfwd>
#include <memory>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>


namespace vt { namespace trace {

struct vt_gzFile;

struct Trace {
  using LogType             = Log;
  using TraceConstantsType  = eTraceConstants;
  using TraceContainersType = TraceContainers;
  using TimeIntegerType     = int64_t;
  using TraceContainerType  = std::vector<std::unique_ptr<LogType>>;
  using TraceStackType      = std::stack<LogType>;

  Trace();
  Trace(std::string const& in_prog_name, std::string const& in_trace_name);

  virtual ~Trace();

  friend struct Log;

  std::string getTraceName() const { return full_trace_name_; }
  std::string getSTSName()   const { return full_sts_name_;   }
  std::string getDirectory() const { return full_dir_name_;   }

  void initialize();
  void setupNames(
    std::string const& in_prog_name, std::string const& in_trace_name,
    std::string const& in_dir_name = ""
  );

  void beginProcessing(
    TraceEntryIDType const ep, TraceMsgLenType const len,
    TraceEventIDType const event, NodeType const from_node,
    double const time = getCurrentTime(),
    uint64_t const idx1 = 0, uint64_t const idx2 = 0, uint64_t const idx3 = 0,
    uint64_t const idx4 = 0
  );
  void endProcessing(
    TraceEntryIDType const ep, TraceMsgLenType const len,
    TraceEventIDType const event, NodeType const from_node,
    double const time = getCurrentTime(),
    uint64_t const idx1 = 0, uint64_t const idx2 = 0, uint64_t const idx3 = 0,
    uint64_t const idx4 = 0
  );

  void beginIdle(double const time = getCurrentTime());
  void endIdle(double const time = getCurrentTime());

  UserEventIDType registerUserEventColl(std::string const& name);
  UserEventIDType registerUserEventRoot(std::string const& name);
  UserEventIDType registerUserEventHash(std::string const& name);
  void registerUserEventManual(std::string const& name, UserSpecEventIDType id);

  void addUserEvent(UserEventIDType event);
  void addUserEventManual(UserSpecEventIDType event);
  void addUserEventBracketed(UserEventIDType event, double begin, double end);
  void addUserEventBracketedManual(
    UserSpecEventIDType event, double begin, double end
  );
  void addUserEventBracketedBegin(UserEventIDType event);
  void addUserEventBracketedEnd(UserEventIDType event);
  void addUserEventBracketedManualBegin(UserSpecEventIDType event);
  void addUserEventBracketedManualEnd(UserSpecEventIDType event);
  void addUserNote(std::string const& note);
  void addUserData(int32_t data);
  void addUserBracketedNote(
    double const begin, double const end, std::string const& note,
    TraceEventIDType const event = no_trace_event
  );

  TraceEventIDType messageCreation(
    TraceEntryIDType const ep, TraceMsgLenType const len,
    double const time = getCurrentTime()
  );
  TraceEventIDType messageCreationBcast(
    TraceEntryIDType const ep, TraceMsgLenType const len,
    double const time = getCurrentTime()
  );
  TraceEventIDType messageRecv(
    TraceEntryIDType const ep, TraceMsgLenType const len,
    NodeType const from_node, double const time = getCurrentTime()
  );

  /// Enable logging of events.
  void enableTracing();

  /// Disable logging of events.
  /// Events already logged may still be written to the trace log.
  void disableTracing();

  bool checkEnabled();
  bool checkDynamicRuntimeEnabled();

  void flushTracesFile(bool useGlobalSync);
  void cleanupTracesFile();

  bool inIdleEvent() const;

  static inline double getCurrentTime() {
    return ::vt::timing::Timing::getCurrentTime();
  }

  static inline TimeIntegerType timeToInt(double const time) {
    return static_cast<TimeIntegerType>(time * 1e6);
  }

  friend void insertNewUserEvent(UserEventIDType event, std::string const& name);

private:

  static void traceBeginIdleTrigger();

  // Writes traces to file, optionally flushing.
  // The traces collection specified may be modified.
  static void outputTraces(
    vt_gzFile* file, TraceContainerType& traces,
    size_t start, size_t stop, double start_time, int flush
  );
  static void outputHeader(
    vt_gzFile* file, NodeType const node, double const start
  );
  static void outputFooter(
    vt_gzFile* file, NodeType const node, double const start
  );

  void writeTracesFile(int flush);

  void outputControlFile(std::ofstream& file);

  static bool traceWritingEnabled(NodeType node);
  static bool isStsOutputNode(NodeType node);

  /// Log an event, returning a trace event ID if accepted
  /// or no_trace_event if not accepted (eg. no tracing on node).
  TraceEventIDType logEvent(std::unique_ptr<LogType> log);

private:
  TraceContainerType traces_;
  TraceStackType open_events_;
  TraceEventIDType cur_event_   = 1;
  std::string prog_name_        = "";
  std::string trace_name_       = "";
  bool enabled_                 = true;
  bool idle_begun_              = false;
  double start_time_            = 0.0;
  std::string full_trace_name_  = "";
  std::string full_sts_name_    = "";
  std::string full_dir_name_    = "";
  UserEventRegistry user_event_ = {};
  std::unique_ptr<vt_gzFile> log_file_;
  bool wrote_sts_file_          = false;

  // Virtual index of last event written.
  size_t cur_                   = 0;
  // Virtual index of last event that should be written.
  // This event (and all previous events) are guaranteed
  // to not be on the open event stack and thus deletion of
  // such will not leave dangling pointers.
  size_t cur_stop_              = 0;
};

}} //end namespace vt::trace

namespace vt {

#if backend_check_enabled(trace_enabled)
  extern trace::Trace* theTrace();
#endif

}

#endif /*INCLUDED_TRACE_TRACE_H*/
