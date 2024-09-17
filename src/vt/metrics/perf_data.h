/*
//@HEADER
// *****************************************************************************
//
//                                  perf_data.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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
 * \brief Tracks perf metrics per task
 *
 */
struct PerfData: runtime::component::Component<PerfData>
{
public:
  PerfData();
  ~PerfData();

  void startTaskMeasurement(runnable::RunnableNew* task);
  void stopTaskMeasurement(runnable::RunnableNew* task);
  std::unordered_map<std::string, uint64_t> getTaskMeasurements(runnable::RunnableNew* task);
  void purgeTask(runnable::RunnableNew* task);

  std::unordered_map<std::string, std::pair<uint64_t,uint64_t>> getEventMap() const;
  void startup() override;
  std::string name() override;

  template <typename SerializerT>
  void serialize(SerializerT& s);

private:
  std::unordered_map<std::string, std::pair<uint64_t,uint64_t>> event_map_;
  std::vector<std::string> event_names_;
  std::vector<int> event_fds_;

  void cleanupBeforeAbort();
  static long perf_event_open(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags);
};

}} // end namespace vt::metrics

namespace vt {

extern metrics::PerfData* thePerfData();

} // end namespace vt

#endif /*INCLUDED_VT_METRICS_PERF_DATA_H*/
