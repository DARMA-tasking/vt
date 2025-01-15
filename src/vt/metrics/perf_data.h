/*
//@HEADER
// *****************************************************************************
//
//                                 perf_data.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_METRICS_PERF_DATA_H
#define INCLUDED_VT_METRICS_PERF_DATA_H

#include "vt/config.h"
#include "vt/runtime/component/component_pack.h"
#include "vt/context/context.h"
#include "example_events.h"

#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <cstring>
#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>

namespace vt { namespace metrics {

/** \file */

/**
 * \struct PerfData perf_data.h vt/metrics/perf_data.h
 *
 * \brief Tracks performance metrics per task
 *
 * The PerfData component is responsible for initializing, tracking, and retrieving
 * performance metrics for specific tasks using Linux performance counters.
 */
struct PerfData: runtime::component::Component<PerfData>
{
public:
  /**
   * \brief Constructor for PerfData
   *
   * Initializes performance counters based on the \c VT_EVENTS environment variable,
   * which is a comma seperated list of events available in the events header
   * (example_events.h by default). For example: \c VT_EVENTS="cache-misses,instructions".
   * If \c VT_EVENTS isn't set, will default to measuring instructions.
   * Ensures only valid events are configured.
   */
  PerfData();

  /**
   * \brief Destructor for PerfData
   *
   * Cleans up resources, closing file descriptors associated with performance
   * counters.
   */
  virtual ~PerfData();

  /**
   * \brief Start performance measurement for a task
   *
   * Resets and enables the performance counters associated with the tracked events.
   */
  void startTaskMeasurement();

  /**
   * \brief Stop performance measurement for a task
   *
   * Disables the performance counters associated with the tracked events.
   */
  void stopTaskMeasurement();

  /**
   * \brief Get the measurements collected during the task execution
   *
   * Reads and retrieves the counter values for all tracked events.
   *
   * \return A map of event names to their corresponding measurement values.
   *
   * \throws vtAbort if there is a mismatch in data or an error during reading.
   */
  std::unordered_map<std::string, uint64_t> getTaskMeasurements();

  /**
   * \brief Retrieve the current event map
   *
   * Returns the mapping of event names to their type and configuration values.
   *
   * \return A map of event names to pairs of event type and configuration values.
   */
  std::unordered_map<std::string, std::pair<uint64_t,uint64_t>> getEventMap() const;

  /**
   * \brief Component startup method
   */
  void startup() override;

  /**
   * \brief Get the component name
   *
   * \return The name of the component as a string.
   */
  std::string name() override;

  /**
   * \brief Serialize the PerfData object
   */
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | event_map_
      | event_names_
      | event_fds_;
  }

private:
  /**
   * \brief Map of event names to event type and configuration
   */
  std::unordered_map<std::string, std::pair<uint64_t,uint64_t>> event_map_;

  /**
   * \brief List of event names being tracked
   */
  std::vector<std::string> event_names_;

  /**
   * \brief List of file descriptors associated with performance counters
   */
  std::vector<int> event_fds_;

  /**
   * \brief Cleanup resources before aborting
   *
   * Closes any open file descriptors and clears internal data structures.
   */
  void cleanupBeforeAbort();

  /**
   * \brief Open a performance counter event
   *
   * Wrapper around the syscall to open a performance counter.
   *
   * \param[in] hw_event The performance event attributes.
   * \param[in] pid The process ID to measure (-1 for calling process).
   * \param[in] cpu The CPU to measure (-1 for any CPU).
   * \param[in] group_fd Group file descriptor for event grouping.
   * \param[in] flags Additional flags for the syscall.
   *
   * \return The file descriptor for the event, or -1 on failure.
   */
  static long perfEventOpen(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags);
};

}} // end namespace vt::metrics

namespace vt {

extern metrics::PerfData* thePerfData();

} // end namespace vt

#endif /*INCLUDED_VT_METRICS_PERF_DATA_H*/
