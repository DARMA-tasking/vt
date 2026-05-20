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

#include "vt/metrics/perf_data.h"

#include <cstdlib>
#include <cmath>
#include <limits>
#include <sstream>

namespace {

std::string joinEventNames(std::vector<std::string> const& event_names) {
  std::ostringstream out;
  for (std::size_t i = 0; i < event_names.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << event_names[i];
  }
  return out.str();
}

std::string makePerfOpenError(
  vt::metrics::PerfEventGroupInfo const& group,
  std::string const& event_name,
  std::size_t attempted_group_size,
  int error_number
) {
  auto message =
    "Error opening perf event '" + event_name + "' for group '" +
    group.group_name_ + "' (source=" + group.source_ + ", attempted_size=" +
    std::to_string(attempted_group_size) + ", configured_size=" +
    std::to_string(group.event_names_.size()) + ", events=[" +
    joinEventNames(group.event_names_) + "], pinned=" +
    std::string(group.pinned_ ? "true" : "false") + "): " +
    std::strerror(error_number);

  if (group.event_names_.size() > 1) {
    message +=
      ". The kernel rejected this grouped perf configuration; the group may "
      "be too large or the events may be incompatible on this platform.";
  }

  if (group.source_ == "auto") {
    message +=
      " Try lowering VT_PERF_AUTO_GROUP_MAX_SIZE or providing explicit "
      "VT_EVENTS groups.";
  } else if (group.source_ == "explicit") {
    message +=
      " Try splitting the explicit VT_EVENTS group into smaller groups.";
  }

  return message;
}

uint64_t scaleCounterValue(
  uint64_t value, uint64_t time_enabled, uint64_t time_running
) {
  if (
    time_enabled == 0 or time_running == 0 or time_running >= time_enabled
  ) {
    return value;
  }

  auto const scale = static_cast<long double>(time_enabled) /
    static_cast<long double>(time_running);
  auto const scaled_value = std::llround(static_cast<long double>(value) * scale);

  if (scaled_value < 0) {
    vtAbort("Scaled perf counter value became negative.");
  }

  return static_cast<uint64_t>(scaled_value);
}

} // end anonymous namespace

namespace vt::metrics {

PerfData::PerfData()
  : event_map_(example_event_map)
{
  auto const* event_spec = std::getenv("VT_EVENTS");
  auto const auto_group_enabled = isPerfEnvEnabled("VT_PERF_AUTO_GROUP");
  auto const auto_group_max_size = auto_group_enabled ?
    getPerfGroupMaxSize("VT_PERF_AUTO_GROUP_MAX_SIZE") : 0;
  auto const resolved_spec =
    event_spec == nullptr ? std::string{"instructions"} : std::string{event_spec};

  event_groups_ = resolvePerfEventGroups(
    resolved_spec, event_map_, auto_group_enabled, auto_group_max_size,
    event_names_
  );

  for (auto const& group : event_groups_) {
    GroupState state;
    state.info_ = group;

    for (auto const& event_name : group.event_names_) {
      auto const& descriptor = event_map_.at(event_name);
      struct perf_event_attr pe = {};
      pe.type = descriptor.type;
      pe.size = sizeof(struct perf_event_attr);
      pe.config = descriptor.config;
      pe.disabled = 1;
      pe.exclude_kernel = 1;
      pe.exclude_hv = 1;
      pe.inherit = 1;
      pe.pinned = state.info_.pinned_ and state.leader_fd_ == -1 ? 1 : 0;
      pe.read_format = PERF_FORMAT_GROUP | PERF_FORMAT_TOTAL_TIME_ENABLED |
        PERF_FORMAT_TOTAL_TIME_RUNNING | PERF_FORMAT_ID;

      int const group_fd = state.leader_fd_ == -1 ? -1 : state.leader_fd_;
      int const fd = perfEventOpen(&pe, 0, -1, group_fd, PERF_FLAG_FD_CLOEXEC);
      if (fd == -1) {
        auto const error_number = errno;
        cleanupBeforeAbort();
        vtAbort(makePerfOpenError(
          state.info_, event_name, state.event_ids_.size() + 1, error_number
        ));
      }

      open_fds_.push_back(fd);

      if (state.leader_fd_ == -1) {
        state.leader_fd_ = fd;
      }

      uint64_t event_id = 0;
      if (ioctl(fd, PERF_EVENT_IOC_ID, &event_id) == -1) {
        cleanupBeforeAbort();
        vtAbort(
          "Failed to retrieve perf event id for: " + event_name +
          ". Error: " + std::string(std::strerror(errno))
        );
      }

      state.event_ids_.emplace(event_id, event_name);
    }

    group_states_.push_back(std::move(state));
  }
}

PerfData::~PerfData() {
  for (int fd : open_fds_) {
    if (fd != -1) {
      close(fd);
    }
  }
}

void PerfData::startTaskMeasurement() {
  for (auto const& group_state : group_states_) {
    if (group_state.leader_fd_ != -1) {
      ioctl(group_state.leader_fd_, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
      ioctl(group_state.leader_fd_, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
    }
  }
}

void PerfData::stopTaskMeasurement() {
  for (auto const& group_state : group_states_) {
    if (group_state.leader_fd_ != -1) {
      ioctl(group_state.leader_fd_, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
    }
  }
}

std::unordered_map<std::string, uint64_t> PerfData::getTaskMeasurements() {
  std::unordered_map<std::string, uint64_t> measurements;

  if (group_states_.size() != event_groups_.size()) {
    vtAbort("Mismatch between opened event groups and configured event groups.");
  }

  for (auto const& group_state : group_states_) {
    auto const expected_events = group_state.info_.event_names_.size();
    std::vector<uint64_t> buffer(3 + (expected_events * 2), 0);
    auto const expected_bytes = static_cast<ssize_t>(buffer.size() * sizeof(uint64_t));
    auto const bytes_read = read(
      group_state.leader_fd_, buffer.data(), static_cast<size_t>(expected_bytes)
    );

    if (bytes_read == -1) {
      vtAbort(
        "Failed to read perf event group data for: " + group_state.info_.group_name_ +
        ". Error: " + std::string(std::strerror(errno))
      );
    }

    if (bytes_read != expected_bytes) {
      vtAbort(
        "Incomplete read for perf event group: " + group_state.info_.group_name_ +
        ". Expected " + std::to_string(expected_bytes) + " bytes, but got " +
        std::to_string(bytes_read)
      );
    }

    auto const event_count = static_cast<size_t>(buffer[0]);
    auto const time_enabled = buffer[1];
    auto const time_running = buffer[2];

    if (event_count != expected_events) {
      vtAbort(
        "Mismatch in grouped perf read for: " + group_state.info_.group_name_ +
        ". Expected " + std::to_string(expected_events) + " events, but got " +
        std::to_string(event_count)
      );
    }

    for (size_t i = 0; i < event_count; ++i) {
      auto const value = buffer[3 + (i * 2)];
      auto const event_id = buffer[4 + (i * 2)];
      auto iter = group_state.event_ids_.find(event_id);
      if (iter == group_state.event_ids_.end()) {
        vtAbort(
          "Unknown perf event id returned for group: " +
          group_state.info_.group_name_
        );
      }

      measurements[iter->second] = scaleCounterValue(
        value, time_enabled, time_running
      );
    }
  }

  if (measurements.size() != event_names_.size()) {
    vtAbort("Mismatch between measured events and configured event names.");
  }

  return measurements;
}

std::unordered_map<std::string, PerfEventDescriptor> PerfData::getEventMap() const {
  return event_map_;
}

std::vector<PerfEventGroupInfo> PerfData::getEventGroups() const {
  return event_groups_;
}

void PerfData::startup() { event_map_ = example_event_map; }

std::string PerfData::name() { return "PerfData"; }

void PerfData::cleanupBeforeAbort() {
  for (int fd : open_fds_) {
    if (fd != -1) {
      close(fd);
    }
  }
  open_fds_.clear();
  group_states_.clear();
}

long PerfData::perfEventOpen(struct perf_event_attr *hw_event, pid_t pid, int cpu, int group_fd, unsigned long flags) {
  return syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
}

} // end namespace vt::metrics
