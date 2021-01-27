/*
//@HEADER
// *****************************************************************************
//
//                                trace_lite.h
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

#if !defined INCLUDED_TRACE_TRACE_LITE_H
#define INCLUDED_TRACE_TRACE_LITE_H

#include "vt/configs/features/features_enableif.h"
#include "vt/trace/trace_common.h"
#include "vt/trace/trace_log.h"
#include "vt/trace/trace_user_event.h"
#include "vt/timing/timing.h"
#include "vt/context/context.h"

#include <string>
#include <queue>
#include <vector>
#include <mpi.h>

namespace vt { namespace trace {

struct vt_gzFile;
struct Trace;

struct TraceLite  {
  using LogType             = Log;
  using TraceConstantsType  = eTraceConstants;
  using TimeIntegerType     = int64_t;
  using EventHoldStackType  = std::vector<std::size_t>;

  using TraceContainerType  = std::queue<LogType>;
  // Although should be used mostly as a stack, vector is exposed to enable
  // the use of a synthetic pop-push to maintain the stack around idle.
  using TraceStackType      = std::vector<LogType>;

  /**
   * \internal \brief System call to construct the trace component
   *
   * \param[in] in_prog_name the program name
   */
  TraceLite(std::string const& in_prog_name);

  virtual ~TraceLite();

  /**
   * \brief Get the trace file name for this node
   *
   * \return the file name
   */
  std::string getTraceName() const { return full_trace_name_; }

  /**
   * \brief Get the sts file name that holds meta-data
   *
   * \return the sts file name
   */
  std::string getSTSName()   const { return full_sts_name_;   }

  /**
   * \brief Get the trace directory for output
   *
   * \return the directory path
   */
  std::string getDirectory() const { return full_dir_name_;   }

#if vt_check_enabled(trace_only)
  /**
   * \brief Initialize trace module in stand-alone mode. This will create
   * stripped-down version of runtime and initialize components needed for
   * tracing MPI calls
   *
   * \param[in] comm MPI communicator type
   */
  void initializeStandalone(MPI_Comm comm);

  /**
   * \brief Cleanup all components initialized for standalone mode
   */
  void finalizeStandalone();
#endif

  /**
   * \brief Collectively register a user event
   *
   * \note For users, it is recommended that the free function be called
   * \c registerEventCollective
   *
   * \param[in] name name for the user event
   *
   * \return the user event ID
   */
  UserEventIDType registerUserEventColl(std::string const& name);

  /**
   * \internal \brief Setup the file names for output
   *
   * \param[in] in_prog_name the program name
   */
  void setupNames(std::string const& in_prog_name);

  /**
   * \brief Enable logging of events at runtime
   */
  void enableTracing();

  /**
   * \brief Disable logging of events.
   *
   * \note Events already logged may still be written to the trace log.
   */
  void disableTracing();

  /**
   * \brief Log a bracketed user event with start and end time
   *
   * \param[in] event the ID for the sts file
   * \param[in] begin the begin time
   * \param[in] end the end time
   */
  void addUserEventBracketed(UserEventIDType event, double begin, double end);

  /**
   * \brief Log a user bracketed event with a note
   *
   * \param[in] begin the begin time
   * \param[in] end the end time
   * \param[in] note the note to log
   * \param[in] event the event ID
   */
  void addUserBracketedNote(
    double const begin, double const end, std::string const& note,
    TraceEventIDType const event = no_trace_event
  );

 /**
   * \brief Scheduler trigger for \c sched::SchedulerEvent::BeginIdle
   *
   * \param[in] time time it begins idle
   */
  void beginIdle(double const time = getCurrentTime());

  /**
   * \brief Scheduler trigger for \c sched::SchedulerEvent::EndIdle
   *
   * \param[in] time time it ends idle
   */
  void endIdle(double const time = getCurrentTime());

  /**
   * \internal \brief Check if tracing is enabled
   *
   * \param[in] is_end_event whether the event that is being considering to
   * write out in the calling context is actually an end event that needs to be
   * closed
   *
   * \return whether tracing is enabled
   */
  bool checkDynamicRuntimeEnabled(bool is_end_event = false);

  /**
   * \brief Flush traces to file
   *
   * \param[in] useGlobalSync whether a global sync should be invoked before
   * flushing output
   */
  void flushTracesFile(bool useGlobalSync = false);

  /**
   * \internal \brief Cleanup traces data, write to disk and close
   */
  void cleanupTracesFile();

  /**
   * \brief Check if trace is in a idle event
   *
   * \return whether in an idle eveent
   */
  bool inIdleEvent() const;

  /**
   * \brief Get the current time
   *
   * \return query the current clock time
   */
  static inline double getCurrentTime() {
    return ::vt::timing::Timing::getCurrentTime();
  }

  /**
   * \brief Convert time in seconds to integer in microseconds
   *
   * \param[in] time the time in seconds as double
   *
   * \return time in microsecond as integer
   */
  static inline TimeIntegerType timeToMicros(double const time) {
    return static_cast<TimeIntegerType>(time * 1e6);
  }

protected:
  /**
   * \brief Emit a 'stop' trace for previous open event or a '[re]start' trace
   * for a reactivated open event. This assists with output flattening.
   *
   * \param[in] time the time
   * \param[in] type type of event to emit
   */
  void emitTraceForTopProcessingEvent(
    double const time, TraceConstantsType const type
  );

  /**
   * \brief Writes traces to file, optionally flushing. The traces collection is
   * modified.
   *
   * \param[in] file the gzip file to write to
   * \param[in] traces the container of collected traces
   * \param[in] start_time the start time
   * \param[in] flush the flush mode
   */
  static void outputTraces(
    vt_gzFile* file, TraceContainerType& traces,
    double start_time, int flush
  );

  /**
   * \brief Output the tracing header
   *
   * \param[in] file the gzip file
   * \param[in] node the node outputting on
   * \param[in] start the start time
   */
  static void outputHeader(
    vt_gzFile* file, NodeType const node, double const start
  );

  /**
   * \brief Output the tracing footer
   *
   * \param[in] file the gzip file
   * \param[in] node the node outputting on
   * \param[in] start the start time
   */
  static void outputFooter(
    vt_gzFile* file, NodeType const node, double const start
  );

  /**
   * \brief Write traces to file
   *
   * \param[in] flush the flush mode
   * \param[in] is_incremental_flush whether this is an incremental flush
   */
  void writeTracesFile(int flush, bool is_incremental_flush);

  /**
   * \brief Output the sts (control) file
   *
   * \param[in] file the file
   */
  void outputControlFile(std::ofstream& file);

  /**
   * \brief Check if tracing is enabled on a certain node
   *
   * \param[in] node the node
   *
   * \return whether it is enabled
   */
  static bool traceWritingEnabled(NodeType node);

  /**
   * \brief Check if a node will output the sts file
   *
   * \param[in] node the node
   *
   * \return whether it will output
   */
  static bool isStsOutputNode(NodeType node);

  /**
   * \brief Log an event, returning a trace event ID if accepted
   * or \c no_trace_event if not accepted (eg. no tracing on node).
   * The log object is invalidated after the call.
   *
   * \param[in] log the entity to log
   *
   * \return the trace event ID for that new log
   */
  TraceEventIDType logEvent(LogType&& log);

  /**
   * \brief Get the current traces size
   *
   * \return computed bytes used for tracing (lower bound)
   */
  std::size_t getTracesSize() const {
    return traces_.size() * sizeof(Log);
  }

protected:
  /*
   * Incremental flush mode for zlib. Not set here with zlib constants to reduce
   * header dependencies.
   */
  int incremental_flush_mode = 0;

  UserEventRegistry user_event_ = {};
  EventHoldStackType event_holds_;
  TraceStackType open_events_;
  TraceContainerType traces_;
  TraceEventIDType cur_event_   = 1;
  UserEventIDType flush_event_  = no_user_event_id;
  bool enabled_                 = true;
  double start_time_            = 0.0;
  std::string prog_name_        = "";
  std::string trace_name_       = "";
  std::string full_trace_name_  = "";
  std::string full_sts_name_    = "";
  std::string full_dir_name_    = "";
  bool wrote_sts_file_          = false;
  size_t trace_write_count_     = 0;
  bool standalone_initalized_   = false;
  bool trace_enabled_cur_phase_ = true;
  bool idle_begun_              = false;
  std::unique_ptr<vt_gzFile> log_file_;
};

}} //end namespace vt::trace

namespace vt {

#if vt_check_enabled(trace_only)
  extern trace::TraceLite* theTrace();
#elif vt_check_enabled(trace_enabled)
  extern trace::Trace* theTrace();
#endif

}

#endif /*INCLUDED_TRACE_TRACE_LITE_H*/
