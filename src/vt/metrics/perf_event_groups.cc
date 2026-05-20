/*
//@HEADER
// *****************************************************************************
//
//                             perf_event_groups.cc
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

#include "vt/config.h"
#include "vt/metrics/perf_event_groups.h"

#include <cctype>
#include <cstdlib>
#include <limits>

namespace {

struct ParsedEventGroup {
  std::vector<std::string> event_names_;
  bool explicit_group_ = false;
  bool pinned_ = false;
};

struct AutoGroupState {
  std::size_t current_index_ = 0;
  std::size_t chunk_count_ = 0;
};

std::string trim(std::string value) {
  auto const first = value.find_first_not_of(" \t\n\r");
  if (first == std::string::npos) {
    return {};
  }

  auto const last = value.find_last_not_of(" \t\n\r");
  return value.substr(first, last - first + 1);
}

std::pair<std::string, bool> splitPinnedSuffix(std::string token) {
  auto trimmed = trim(std::move(token));
  if (trimmed.empty()) {
    return {{}, false};
  }

  auto const marker_pos = trimmed.find('!');
  if (marker_pos == std::string::npos) {
    return {trimmed, false};
  }

  if (
    marker_pos != trimmed.size() - 1 or
    trimmed.find('!', marker_pos + 1) != std::string::npos
  ) {
    vtAbort(
      "Malformed VT_EVENTS specification: pin marker must appear once at the end of an event or group."
    );
  }

  auto event_name = trim(trimmed.substr(0, marker_pos));
  if (event_name.empty()) {
    vtAbort(
      "Malformed VT_EVENTS specification: pin marker must follow an event or grouped event list."
    );
  }

  return {event_name, true};
}

void validateEventName(
  std::string const& event_name,
  vt::metrics::PerfEventDescriptorMap const& event_map,
  std::unordered_map<std::string, bool>& seen_events
) {
  if (event_map.find(event_name) == event_map.end()) {
    vtAbort("Event name isn't in known perf events map: " + event_name);
  }

  if (seen_events[event_name]) {
    vtAbort("Duplicate perf event configured: " + event_name);
  }

  seen_events[event_name] = true;
}

std::vector<ParsedEventGroup> parseEventGroups(std::string const& event_spec) {
  std::vector<ParsedEventGroup> parsed_groups;
  std::vector<std::string> current_group;
  std::string current_token;
  bool in_group = false;
  bool after_group = false;
  bool saw_delimiter = false;

  auto flushToken = [&]() {
    auto const token = trim(current_token);
    current_token.clear();
    return token;
  };

  for (char const ch : event_spec) {
    if (after_group) {
      if (std::isspace(static_cast<unsigned char>(ch))) {
        continue;
      }

      if (ch == '!') {
        if (parsed_groups.empty() or not parsed_groups.back().explicit_group_) {
          vtAbort(
            "Malformed VT_EVENTS specification: pin marker must follow a grouped event list or event name."
          );
        }

        if (parsed_groups.back().pinned_) {
          vtAbort(
            "Malformed VT_EVENTS specification: duplicate pin marker after grouped events."
          );
        }

        parsed_groups.back().pinned_ = true;
        continue;
      }

      if (ch == ',') {
        after_group = false;
        saw_delimiter = true;
        continue;
      }

      vtAbort(
        "Malformed VT_EVENTS specification: missing comma after grouped events."
      );
    }

    if (ch == '{') {
      if (in_group) {
        vtAbort(
          "Malformed VT_EVENTS specification: nested event groups are not supported."
        );
      }

      if (not trim(current_token).empty()) {
        vtAbort(
          "Malformed VT_EVENTS specification: missing comma before grouped events."
        );
      }

      current_token.clear();
      current_group.clear();
      in_group = true;
      saw_delimiter = false;
      continue;
    }

    if (ch == '}') {
      if (not in_group) {
        vtAbort("Malformed VT_EVENTS specification: unmatched closing brace.");
      }

      auto const token = flushToken();
      auto const parsed_token = splitPinnedSuffix(token);
      if (parsed_token.first.empty()) {
        vtAbort(
          "Malformed VT_EVENTS specification: empty event in grouped configuration."
        );
      }

      if (parsed_token.second) {
        vtAbort(
          "Malformed VT_EVENTS specification: pin a grouped event list with a trailing '!' after the closing brace."
        );
      }

      current_group.push_back(parsed_token.first);
      parsed_groups.push_back({current_group, true, false});
      current_group.clear();
      in_group = false;
      after_group = true;
      saw_delimiter = false;
      continue;
    }

    if (ch == ',') {
      auto const token = flushToken();
      auto const parsed_token = splitPinnedSuffix(token);

      if (in_group) {
        if (parsed_token.first.empty()) {
          vtAbort(
            "Malformed VT_EVENTS specification: empty event in grouped configuration."
          );
        }

        if (parsed_token.second) {
          vtAbort(
            "Malformed VT_EVENTS specification: pin a grouped event list with a trailing '!' after the closing brace."
          );
        }

        current_group.push_back(parsed_token.first);
      } else {
        if (parsed_token.first.empty()) {
          vtAbort("Malformed VT_EVENTS specification: empty event in event list.");
        }

        parsed_groups.push_back({{parsed_token.first}, false, parsed_token.second});
      }

      saw_delimiter = true;
      continue;
    }

    if (not std::isspace(static_cast<unsigned char>(ch))) {
      saw_delimiter = false;
    }

    current_token.push_back(ch);
  }

  if (in_group) {
    vtAbort("Malformed VT_EVENTS specification: missing closing brace.");
  }

  if (after_group) {
    return parsed_groups;
  }

  auto const token = flushToken();
  auto const parsed_token = splitPinnedSuffix(token);
  if (parsed_token.first.empty()) {
    if (parsed_groups.empty()) {
      vtAbort("Malformed VT_EVENTS specification: no events were configured.");
    }

    if (saw_delimiter) {
      vtAbort("Malformed VT_EVENTS specification: trailing comma in event list.");
    }
  } else {
    parsed_groups.push_back({{parsed_token.first}, false, parsed_token.second});
  }

  return parsed_groups;
}

void appendAutoGroupEvent(
  std::vector<vt::metrics::PerfEventGroupInfo>& resolved_groups,
  std::unordered_map<std::string, AutoGroupState>& auto_group_states,
  std::string const& auto_name,
  std::string const& event_name,
  std::size_t auto_group_max_size
) {
  auto iter = auto_group_states.find(auto_name);
  if (iter == auto_group_states.end()) {
    auto const new_index = resolved_groups.size();
    auto_group_states.emplace(auto_name, AutoGroupState{new_index, 0});
    resolved_groups.push_back({auto_name, "auto", {event_name}, false});
    return;
  }

  auto& state = iter->second;
  auto& current_group = resolved_groups[state.current_index_];
  auto const max_size_enabled = auto_group_max_size > 0;
  auto const current_group_full =
    max_size_enabled and current_group.event_names_.size() >= auto_group_max_size;

  if (not current_group_full) {
    current_group.event_names_.push_back(event_name);
    return;
  }

  if (state.chunk_count_ == 0) {
    current_group.group_name_ = auto_name + "-0";
  }

  ++state.chunk_count_;
  state.current_index_ = resolved_groups.size();
  resolved_groups.push_back({
    auto_name + "-" + std::to_string(state.chunk_count_),
    "auto",
    {event_name},
    false
  });
}

} // end anonymous namespace

namespace vt::metrics {

bool isPerfEnvEnabled(char const* env_name) {
  auto const* value = std::getenv(env_name);
  if (value == nullptr) {
    return false;
  }

  std::string normalized(value);
  for (char& ch : normalized) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }

  return normalized == "1" or normalized == "true" or normalized == "on" or
    normalized == "yes";
}

std::size_t getPerfGroupMaxSize(char const* env_name) {
  auto const* value = std::getenv(env_name);
  if (value == nullptr) {
    return 0;
  }

  char* end = nullptr;
  errno = 0;
  auto const parsed_value = std::strtoull(value, &end, 10);

  if (
    errno != 0 or end == value or end == nullptr or *end != '\0' or
    parsed_value == 0 or parsed_value > std::numeric_limits<std::size_t>::max()
  ) {
    vtAbort(
      std::string(env_name) +
      " must be set to a positive integer when provided. Got: " + value
    );
  }

  return static_cast<std::size_t>(parsed_value);
}

std::vector<PerfEventGroupInfo> resolvePerfEventGroups(
  std::string const& event_spec, PerfEventDescriptorMap const& event_map,
  bool auto_group, std::size_t auto_group_max_size,
  std::vector<std::string>& event_names
) {
  auto const parsed_groups = parseEventGroups(event_spec);
  std::vector<PerfEventGroupInfo> resolved_groups;
  std::unordered_map<std::string, bool> seen_events;
  std::unordered_map<std::string, AutoGroupState> auto_group_states;
  std::size_t explicit_group_index = 0;

  for (auto const& parsed_group : parsed_groups) {
    for (auto const& event_name : parsed_group.event_names_) {
      validateEventName(event_name, event_map, seen_events);
      event_names.push_back(event_name);
    }

    if (parsed_group.explicit_group_) {
      resolved_groups.push_back({
        "explicit-" + std::to_string(explicit_group_index++),
        "explicit",
        parsed_group.event_names_,
        parsed_group.pinned_
      });
      continue;
    }

    auto const& event_name = parsed_group.event_names_.front();
    auto const& descriptor = event_map.at(event_name);

    if (parsed_group.pinned_) {
      resolved_groups.push_back({
        "singleton-" + event_name,
        "singleton",
        {event_name},
        true
      });
      continue;
    }

    if (auto_group and not descriptor.auto_group_.empty()) {
      auto const auto_name = "auto-" + descriptor.auto_group_;
      appendAutoGroupEvent(
        resolved_groups, auto_group_states, auto_name, event_name,
        auto_group_max_size
      );
    } else {
      resolved_groups.push_back({
        "singleton-" + event_name,
        "singleton",
        {event_name},
        false
      });
    }
  }

  return resolved_groups;
}

} // end namespace vt::metrics