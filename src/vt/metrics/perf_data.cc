/*
//@HEADER
// *****************************************************************************
//
//                                 perf_data.cc
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

#include "perf_data.h"

namespace vt { namespace metrics {

PerfData::PerfData()
  : event_map_(example_event_map)
{
  char const* env_p = getenv("VT_EVENTS");

  // Check if the environment variable is set
  if (env_p == nullptr) {
    event_names_.push_back("instructions");
  } else {
    std::string env_str(env_p);
    std::stringstream ss(env_str);
    std::string item;

    while (std::getline(ss, item, ','))
    {
      event_names_.push_back(item);
    }
  }

  for (const auto &event_name : event_names_) {
    if (event_map_.find(event_name) == event_map_.end()) {
      cleanupBeforeAbort();
      vtAbort("Event name isn't in known perf events map: " + event_name);
    }
  }

  // Initialize perf events once and store file descriptors
  for (const auto &event_name : event_names_) {
    struct perf_event_attr pe = {};
    pe.type = event_map_.at(event_name).first;
    pe.size = sizeof(struct perf_event_attr);
    pe.config = event_map_.at(event_name).second;

    pe.disabled = 1;
    pe.exclude_kernel = 1;
    pe.exclude_hv = 1;
    pe.inherit = 1; // Ensure event is inherited by threads

    if (event_name == "instructions") {
      pe.pinned = 1;
    }

    int fd = perfEventOpen(&pe, 0, -1, -1, PERF_FLAG_FD_CLOEXEC);
    if (fd == -1) {
      cleanupBeforeAbort();
      vtAbort("Error opening perf event: " + std::string(strerror(errno)));
    }

    event_fds_.push_back(fd);
  }
}

PerfData::~PerfData() {
  for (int fd : event_fds_) {
    if (fd != -1) {
      close(fd);
    }
  }
}

void PerfData::startTaskMeasurement() {
  for (int fd : event_fds_) {
    if (fd != -1) {
      ioctl(fd, PERF_EVENT_IOC_RESET, 0);
      ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
    }
  }
}

void PerfData::stopTaskMeasurement() {
  for (int fd : event_fds_) {
    if (fd != -1) {
      ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
    }
  }
}

std::unordered_map<std::string, uint64_t> PerfData::getTaskMeasurements() {
  std::unordered_map<std::string, uint64_t> measurements;

  if (event_fds_.size() != event_names_.size()) {
    vtAbort("Mismatch between event_fds_ and event_names_ sizes.");
  }

  for (size_t i = 0; i < event_fds_.size(); ++i) {
    int64_t buffer = 0;

    if (event_fds_[i] != -1) {
      if (sizeof(buffer) != sizeof(int64_t)) {
        vtAbort("Buffer size mismatch: expected " + std::to_string(sizeof(int64_t)) +
                " bytes, but got " + std::to_string(sizeof(buffer)));
      }

      ssize_t bytesRead = read(event_fds_[i], &buffer, sizeof(buffer));

      if (bytesRead == sizeof(buffer)) {
        if (buffer < 0) {
          vtAbort("Negative value read from perf event data for: " + event_names_[i] +
                  ". Value: " + std::to_string(buffer));
        }

        uint64_t count = static_cast<uint64_t>(buffer);
        measurements[event_names_[i]] = count;
      } else if (bytesRead == -1) {
        vtAbort("Failed to read perf event data for: " + event_names_[i] +
                ". Error: " + std::strerror(errno));
      } else {
        vtAbort("Incomplete read for: " + event_names_[i] +
                ". Expected " + std::to_string(sizeof(buffer)) +
                " bytes, but got " + std::to_string(bytesRead));
      }
    } else {
      vtAbort("Invalid file descriptor for: " + event_names_[i]);
    }
  }

  return measurements;
}

std::unordered_map<std::string, std::pair<uint64_t,uint64_t>> PerfData::getEventMap() const { return event_map_; }

void PerfData::startup() { event_map_ = example_event_map; }

std::string PerfData::name() { return "PerfData"; }

void PerfData::cleanupBeforeAbort() {
  for (int fd : event_fds_) {
    if (fd != -1) {
      close(fd);
    }
  }
  event_fds_.clear();
}

long PerfData::perfEventOpen(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags) {
  return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

}} // end namespace vt::metrics
