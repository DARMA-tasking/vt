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
#include <string>
#include <stack>
#include <queue>

namespace vt { namespace trace {

struct vt_gzFile;

/// Tracking information for beginProcessing/endProcessing.
struct TraceProcessingTag {

  TraceProcessingTag() = default;
  TraceProcessingTag(TraceProcessingTag const&) = default;
  TraceProcessingTag& operator=(TraceProcessingTag const&) = default;

  friend struct Trace;

private:

  TraceProcessingTag(
    TraceEntryIDType ep, TraceEventIDType event
  ) : ep_(ep), event_(event)
  {}

  TraceEntryIDType ep_ = trace::no_trace_entry_id;
  TraceEventIDType event_ = trace::no_trace_event;
};

struct Trace {
  using LogType             = Log;
  using TraceConstantsType  = eTraceConstants;
  using TimeIntegerType     = int64_t;
  using TraceContainerType  = std::queue<LogType>;
  // Although should be used mostly as a stack, vector is exposed to enable
  // the use of a synthetic pop-push to maintain the stack around idle.
  using TraceStackType      = std::vector<LogType>;
  using EventHoldStackType  = std::vector<std::size_t>;

  Trace();
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

  /// Initiate a paired event.
  /// Currently endProcessing MUST be called in the opposite
  /// order of beginProcessing.
  TraceProcessingTag beginProcessing(
    TraceEntryIDType const ep, TraceMsgLenType const len,
    TraceEventIDType const event, NodeType const from_node,
    double const time = getCurrentTime(),
    uint64_t const idx1 = 0, uint64_t const idx2 = 0, uint64_t const idx3 = 0,
    uint64_t const idx4 = 0
  ) {
    return beginProcessing(ep, len, event, from_node, idx1, idx2, idx3, idx4, time);
  }

  TraceProcessingTag beginProcessing(
     TraceEntryIDType const ep, TraceMsgLenType const len,
     TraceEventIDType const event, NodeType const from_node,
     uint64_t const idx1, uint64_t const idx2,
     uint64_t const idx3, uint64_t const idx4,
     double const time = getCurrentTime()
  );

  /// Finalize a paired event.
  /// The processing_tag value comes from beginProcessing.
  void endProcessing(
    TraceProcessingTag const& processing_tag,
    double const time = getCurrentTime()
  );

  void endProcessing(
    TraceEntryIDType const ep, TraceMsgLenType const len,
    TraceEventIDType const event, NodeType const from_node,
    double const time = getCurrentTime(),
    uint64_t const idx1 = 0, uint64_t const idx2 = 0, uint64_t const idx3 = 0,
    uint64_t const idx4 = 0
  );

  void beginSchedulerLoop();
  void endSchedulerLoop();

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

  bool checkDynamicRuntimeEnabled();

  void loadAndBroadcastSpec();
  void setTraceEnabledCurrentPhase(PhaseType cur_phase);

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

  // Emit a 'stop' trace for previous open event or a '[re]start' trace
  // for a reactivated open event. This assists with output flattening.
  // The event must be in the current scheduler loop's event stack depth.
  void emitTraceForTopProcessingEvent(
    double const time, TraceConstantsType const type
  );

  // Writes traces to file, optionally flushing.
  // The traces collection is modified.
  static void outputTraces(
    vt_gzFile* file, TraceContainerType& traces,
    double start_time, int flush
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
  /// The log object is invalidated after the call.
  TraceEventIDType logEvent(LogType&& log);

private:
  /*
   * Incremental flush mode for zlib. Not set here with zlib constants to reduce
   * header dependencies.
   */
  int incremental_flush_mode = 0;

private:
  TraceContainerType traces_;
  TraceStackType open_events_;
  EventHoldStackType event_holds_;

  TraceEventIDType cur_event_   = 1;
  bool enabled_                 = true;
  bool idle_begun_              = false;
  double start_time_            = 0.0;
  TraceProcessingTag cur_loop_event_;
  UserEventRegistry user_event_ = {};

  std::string prog_name_        = "";
  std::string trace_name_       = "";
  std::string full_trace_name_  = "";
  std::string full_sts_name_    = "";
  std::string full_dir_name_    = "";
  std::unique_ptr<vt_gzFile> log_file_;
  bool wrote_sts_file_          = false;
  size_t trace_write_count_     = 0;

  ObjGroupProxyType spec_proxy_ = vt::no_obj_group;
  bool trace_enabled_cur_phase_ = true;
};

}} //end namespace vt::trace

namespace vt {

#if backend_check_enabled(trace_enabled)
  extern trace::Trace* theTrace();
#endif

}

#endif /*INCLUDED_TRACE_TRACE_H*/
