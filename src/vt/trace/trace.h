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
#include "vt/runtime/component/component_pack.h"

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

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | ep_
      | event_;
  }

private:

  TraceProcessingTag(
    TraceEntryIDType ep, TraceEventIDType event
  ) : ep_(ep), event_(event)
  {}

  TraceEntryIDType ep_ = trace::no_trace_entry_id;
  TraceEventIDType event_ = trace::no_trace_event;
};

/**
 * \struct Trace
 *
 * \brief A optional VT component that traces execution on multiple node for
 * performance analysis.
 *
 * Traces distributed execution on every node for registered handlers, user
 * events, and MPI invocations, to produce traces that can be analyzed after the
 * program terminates. Tracks dependencies between handlers for later
 * analysis. VT handlers are automatically traced through registration and
 * dispatch from the scheduler. Through the PMPI interface, MPI events can be
 * traced while VT is not running.
 *
 * Outputs Projections log and sts files, which can be examined by the
 * java Projections tool.
 */
struct Trace : runtime::component::Component<Trace> {
  using LogType             = Log;
  using TraceConstantsType  = eTraceConstants;
  using TimeIntegerType     = int64_t;
  using TraceContainerType  = std::queue<LogType>;
  // Although should be used mostly as a stack, vector is exposed to enable
  // the use of a synthetic pop-push to maintain the stack around idle.
  using TraceStackType      = std::vector<LogType>;
  using EventHoldStackType  = std::vector<std::size_t>;

  /**
   * \internal \brief System call to construct the trace component
   *
   * \param[in] in_prog_name the program name
   */
  Trace(std::string const& in_prog_name);

  virtual ~Trace();

  std::string name() override { return "Trace"; }

  friend struct Log;

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

  void initialize() override;
  void startup() override;
  void finalize() override;

  /**
   * \brief Initialize trace module in stand-alone mode. This will create
   * stripped-down version of runtime and initialize components needed for
   * tracing MPI calls
   *
   * \param[in] comm MPI communicator type
   * \param[in] flush_size Flush output trace every (flush_size) trace records
   */
  void initializeStandalone(MPI_Comm comm, int32_t flush_size);

  /**
   * \brief Cleanup all components initialized for standalone mode
   */
  void finalizeStandalone();

  /**
   * \internal \brief Setup the file names for output
   *
   * \param[in] in_prog_name the program name
   */
  void setupNames(std::string const& in_prog_name);

  /**
   * \brief Initiate a paired processing event.
   *
   * Currently \c endProcessing MUST be called in the opposite order of
   * \c beginProcessing
   *
   * \param[in] ep the entry point (registered handler ID)
   * \param[in] len size of message in bytes
   * \param[in] event the associated trace event
   * \param[in] from_node which node instigated this processing
   * \param[in] idx1 (optional) if collection, dimension 1
   * \param[in] idx2 (optional) if collection, dimension 2
   * \param[in] idx3 (optional) if collection, dimension 3
   * \param[in] idx4 (optional) if collection, dimension 4
   * \param[in] time the time this occurred
   *
   * \return a tag to close this processing event
   */
  TraceProcessingTag beginProcessing(
     TraceEntryIDType const ep, TraceMsgLenType const len,
     TraceEventIDType const event, NodeType const from_node,
     uint64_t const idx1 = 0, uint64_t const idx2 = 0,
     uint64_t const idx3 = 0, uint64_t const idx4 = 0,
     double const time = getCurrentTime()
  );

  /**
   * \brief Finalize a paired event.
   *
   * The \c processing_tag value comes from \c beginProcessing.
   *
   * \param[in] processing_tag the matching tag from \c beginProcessing
   * \param[in] time the time this occurred
   */
  void endProcessing(
    TraceProcessingTag const& processing_tag,
    double const time = getCurrentTime()
  );

  /**
   * \brief Scheduler trigger for \c sched::SchedulerEvent::PendingSchedulerLoop
   */
  void pendingSchedulerLoop();

  /**
   * \brief Scheduler trigger for \c sched::SchedulerEvent::BeginSchedulerLoop
   */
  void beginSchedulerLoop();

  /**
   * \brief Scheduler trigger for \c sched::SchedulerEvent::EndSchedulerLoop
   */
  void endSchedulerLoop();

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
   * \brief Register a user event rooted on a single node
   *
   * \note For users, it is recommended that the free function be called
   * \c registerEventRooted
   *
   * \param[in] name name for the user event
   *
   * \return the user event ID
   */
  UserEventIDType registerUserEventRoot(std::string const& name);

  /**
   * \brief Idempotent registration of a user event using a hash of its name
   *
   * \note For users, it is recommended that the free function be called
   * \c registerEventHashed
   *
   * \warning This call can be dangerous because while it does allow impromptu
   * user event type creation, any collisions in the hash will cause multiple
   * events to be conflated to the same event type
   *
   * \param[in] name name for the user event
   *
   * \return the user event ID
   */
  UserEventIDType registerUserEventHash(std::string const& name);

  /**
   * \brief Manually register a user event, directly passing the ID for the sts
   * file
   *
   * \warning This call may be dangerous unless all user events IDs are managed
   * for a given program
   *
   * \param[in] name name for the user event
   * \param[in] id the ID for the sts file
   */
  void registerUserEventManual(std::string const& name, UserSpecEventIDType id);

  /**
   * \brief Log a user event
   *
   * \param[in] event the event ID
   */
  void addUserEvent(UserEventIDType event);

  /**
   * \brief Log a user event generated manually
   *
   * \param[in] event the event ID
   */
  void addUserEventManual(UserSpecEventIDType event);

  /**
   * \brief Log a bracketed user event with start and end time
   *
   * \param[in] event the ID for the sts file
   * \param[in] begin the begin time
   * \param[in] end the end time
   */
  void addUserEventBracketed(UserEventIDType event, double begin, double end);

  /**
   * \brief Log a bracketed user event manually with start and end time
   *
   * \param[in] event the ID for the sts file
   * \param[in] begin the begin time
   * \param[in] end the end time
   */
  void addUserEventBracketedManual(
    UserSpecEventIDType event, double begin, double end
  );

  /**
   * \brief Log the start of a user event that is bracketed
   *
   * \param[in] event the event ID
   */
  void addUserEventBracketedBegin(UserEventIDType event);

  /**
   * \brief Log the end of a user event that is bracketed
   *
   * \param[in] event the event ID
   */
  void addUserEventBracketedEnd(UserEventIDType event);

  /**
   * \brief Log the start of a manual user event that is bracketed
   *
   * \param[in] event  the ID for the sts file
   */
  void addUserEventBracketedManualBegin(UserSpecEventIDType event);

  /**
   * \brief Log the end of a manual user event that is bracketed
   *
   * \param[in] event  the ID for the sts file
   */
  void addUserEventBracketedManualEnd(UserSpecEventIDType event);

  /**
   * \brief Log a user note
   *
   * \param[in] note the note to add
   */
  void addUserNote(std::string const& note);

  /**
   * \brief Log a user note with an integer
   *
   * \param[in] data the integer to add
   */
  void addUserData(int32_t data);

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
   * \brief Log a memory usage event
   *
   * \param[in] memory the amount of memory used
   * \param[in] time the time it occurred
   */
  void addMemoryEvent(
    std::size_t memory,
    double const time = getCurrentTime()
  );

  /**
   * \brief Log a message send
   *
   * \param[in] ep the handler ID
   * \param[in] len the size of the message in bytes
   * \param[in] time the time is was sent
   *
   * \return the trace event ID
   */
  TraceEventIDType messageCreation(
    TraceEntryIDType const ep, TraceMsgLenType const len,
    double const time = getCurrentTime()
  );

  /**
   * \brief Log a message broadcast
   *
   * \param[in] ep the handler ID
   * \param[in] len the size of the message in bytes
   * \param[in] time the time is was sent
   *
   * \return the trace event ID
   */
  TraceEventIDType messageCreationBcast(
    TraceEntryIDType const ep, TraceMsgLenType const len,
    double const time = getCurrentTime()
  );

  /**
   * \brief Log a received message
   *
   * \param[in] ep the handler ID
   * \param[in] len the size of the message in bytes
   * \param[in] from_node node that sent the message
   * \param[in] time the time is was sent
   *
   * \return the trace event ID
   */
  TraceEventIDType messageRecv(
    TraceEntryIDType const ep, TraceMsgLenType const len,
    NodeType const from_node, double const time = getCurrentTime()
  );

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
   * \internal \brief Load and broadcast the trace specification file
   */
  void loadAndBroadcastSpec();

  /**
   * \internal \brief Tell tracing that a new phase has been reached so tracing
   * can be enabled/disabled based on a specification file.
   *
   * \param[in] cur_phase the phase
   */
  void setTraceEnabledCurrentPhase(PhaseType cur_phase);

  /**
   * \brief Flush traces to file
   *
   * \param[in] useGlobalSync whether a global sync should be invoked before
   * flushing output
   */
  void flushTracesFile(bool useGlobalSync);

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

  friend void insertNewUserEvent(UserEventIDType event, std::string const& name);

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | incremental_flush_mode
      | traces_
      | open_events_
      | event_holds_
      | cur_event_
      | enabled_
      | idle_begun_
      | start_time_
      | user_event_
      | prog_name_
      | trace_name_
      | full_trace_name_
      | full_sts_name_
      | full_dir_name_
      | wrote_sts_file_
      | trace_write_count_
      | spec_proxy_
      | trace_enabled_cur_phase_
      | flush_event_
      | between_sched_event_type_
      | between_sched_event_;
  }

private:
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
  UserEventRegistry user_event_ = {};

  std::string prog_name_        = "";
  std::string trace_name_       = "";
  std::string full_trace_name_  = "";
  std::string full_sts_name_    = "";
  std::string full_dir_name_    = "";
  std::unique_ptr<vt_gzFile> log_file_;
  bool wrote_sts_file_          = false;
  size_t trace_write_count_     = 0;
  bool standalone_version_      = false;

  ObjGroupProxyType spec_proxy_ = vt::no_obj_group;
  bool trace_enabled_cur_phase_ = true;
  UserEventIDType flush_event_  = no_user_event_id;

  // Processing event between top-level loops.
  TraceEntryIDType between_sched_event_type_ = no_trace_entry_id;
  TraceProcessingTag between_sched_event_;
};

}} //end namespace vt::trace

namespace vt {

#if vt_check_enabled(trace_enabled)
  extern trace::Trace* theTrace();
#endif

}

#endif /*INCLUDED_TRACE_TRACE_H*/
