/*
//@HEADER
// *****************************************************************************
//
//                                 trace_log.h
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

#if !defined INCLUDED_TRACE_TRACE_LOG_H
#define INCLUDED_TRACE_TRACE_LOG_H

#include "vt/config.h"
#include "vt/trace/trace_common.h"
#include "vt/trace/trace_constants.h"

#include <cstdint>
#include <vector>
#include <memory>
#include <new>

namespace vt { namespace trace {

struct Log final {
  using TraceConstantsType = eTraceConstants;
  using UserDataType       = int32_t;

  enum LogDataType : uint8_t {
    user = 1,
    sys = 2
  };

  union Data {

    // System event data; cannot be used in 'user events'.
    // Type is simple with auto-defined ctor/copy/move..
    // Expected ~40 bytes
    struct SysData {
    public:
      // "Common initial sequence"
      LogDataType data_type = LogDataType::sys;
      //
      TraceMsgLenType msg_len;
      uint64_t idx1;
      uint64_t idx2;
      uint64_t idx3;
      uint64_t idx4;

    private:
      friend struct Log;

      SysData(
        TraceMsgLenType in_msg_len,
        uint64_t in_idx1 = 0, uint64_t in_idx2 = 0,
        uint64_t in_idx3 = 0, uint64_t in_idx4 = 0
      ) : msg_len(in_msg_len),
          idx1(in_idx1), idx2(in_idx2), idx3(in_idx3), idx4(in_idx4)
      {
      }

      SysData() = delete; // prevent ctor looking like value-init
      SysData(SysData const& in) = default;
      SysData(SysData&& in) = default;
      SysData& operator=(SysData const& in) = default;
      SysData& operator=(SysData&& in) = default;

    } sys;

    // User event data; cannot be used in 'normal events'.
    // Placement-new usages for non-trivial std::string type involved.
    // Expected ~30-40 bytes.
    struct UserData {
      // "Common initial sequence"
      LogDataType data_type = LogDataType::user;
      //
      std::string user_note;
      UserDataType user_data;
      UserEventIDType user_event;
      bool user_start;

    private:
      // All except for direct property access via either
      // user_data() or sys_data(), which return references,
      // is internal to Log. No external copies for you.
      friend struct Log;

      UserData(
        std::string const& in_user_note, UserDataType in_user_data,
        UserEventIDType in_user_event, bool in_user_start
      ) : user_note(in_user_note), user_data(in_user_data),
          user_event(in_user_event), user_start(in_user_start)
      {
      }

      // Assigned via placement-new ctors (only)
      UserData() = delete;
      UserData(UserData const&) = default;
      UserData(UserData&&) = default;
      UserData& operator=(UserData const&) = default;
      UserData& operator=(UserData&&) = default;

    } user;

    // Copy based on type.
    Data(Data const& data) {
      if (data.user.data_type == Log::LogDataType::user) {
        new (&user) UserData{data.user};
      } else {
        sys = data.sys;
      }
    }

    // Move based on type
    Data(Data&& data) {
      if (data.user.data_type == Log::LogDataType::user) {
        new (&user) UserData{std::move(data.user)};
      } else {
        sys = std::move(data.sys);
      }
    }

    Data &operator=(Data const& other_data) {
      clean();
      if (other_data.user.data_type == Log::LogDataType::user) {
        new (&user) UserData{other_data.user};
      } else {
        sys = other_data.sys;
      }

      return *this;
    }

    Data &operator=(Data && other_data) {
      clean();
      if (other_data.user.data_type == Log::LogDataType::user) {
        new (&user) UserData{std::move(other_data.user)};
      } else {
        sys = std::move(other_data.sys);
      }

      return *this;
    }

    Data(UserData const& user_data) {
      new (&user) UserData{user_data};
    }

    Data(SysData const& sys_data) : sys(sys_data) {
    }

    ~Data() {
      clean();
    }

  private:

    void clean() {
      // Can access "Common initial sequence" per C++11 6.5.2.3/6
      if (user.data_type == LogDataType::user) {
        // Cleanup placement-new artifacts.
        user.~UserData();
      }
    }
  };

  // [[deprecated]]] - use appropriate ctor
  void setUserNote(std::string const& note) {
    if (data.user.data_type == LogDataType::user) {
      data.user.user_note = note;
    }
  }

  // [[deprecated]] - use appropriate ctor
  void setUserData(UserDataType user_data) {
    if (data.user.data_type == LogDataType::user) {
      data.user.user_data = user_data;
    }
  }

  // No default constructor. Seems wierd? Copy+move all the way..
  // (Copy-constructible and copy-assignable required for dequeue.)
  Log() = delete;
  Log(Log const& in) = default;
  Log(Log&& in) = default;
  Log& operator=(Log const& in) = default;
  Log& operator=(Log&& in) = default;

  // User event
  Log(
    double const in_begin_time, double const in_end_time,
    TraceConstantsType const in_type, std::string const& in_note,
    TraceEventIDType const in_event
  ) : time(in_begin_time), end_time(in_end_time),
      type(in_type), event(in_event),
      data(Data::UserData{in_note, 0, 0, false})
  {
  }

  // User event
  Log(
      double const in_time, TraceConstantsType const in_type,
      std::string const& in_note, UserDataType in_data
  ) : time(in_time), type(in_type),
      data(Data::UserData{in_note, 0, 0, false})
  {
  }

  // User event
  Log(
    double const in_time, TraceConstantsType const in_type,
    NodeType in_node,
    UserEventIDType in_user_event, bool in_user_start
  ) : time(in_time), type(in_type),
      node(in_node),
      data(Data::UserData{std::string{}, 0, in_user_event, in_user_start})
  {
  }

  // Used for idle
  Log(
    double const in_time, TraceConstantsType const in_type,
    NodeType in_node
  ) : time(in_time), type(in_type),
      node(in_node),
      data(Data::SysData{0})
  {
  }

  // Used for messages
  Log(
    double const in_time, TraceEntryIDType const in_ep, TraceConstantsType const in_type,
    NodeType in_node,
    TraceMsgLenType const in_msg_len
  ) : time(in_time), type(in_type), ep(in_ep),
      node(in_node),
      data(Data::SysData{in_msg_len})
  {
  }

  // Generate paired begin/end logs (copies with few changes)
  // NOT VALID FOR USER EVENTS
  Log(
    Log const& in, double in_time, TraceConstantsType in_type
  ) : time(in_time), type(in_type), ep(in.ep), event(in.event),
      node(in.node),
      data(in.sys_data())
  {
  }

  Log(
    double in_time, TraceEntryIDType in_ep, TraceConstantsType in_type,
    TraceEventIDType in_event, TraceMsgLenType in_msg_len, NodeType in_node,
    uint64_t in_idx1, uint64_t in_idx2, uint64_t in_idx3, uint64_t in_idx4
  ) : time(in_time), type(in_type), ep(in_ep), event(in_event),
      node(in_node),
      data(Data::SysData{in_msg_len, in_idx1, in_idx2, in_idx3, in_idx4})
  {
  }

  inline Data::UserData const& user_data() const {
    assert(data.user.data_type == LogDataType::user && "Expecting user data-type");
    return data.user;
  }

  inline Data::SysData const& sys_data() const {
    assert(data.sys.data_type == LogDataType::sys && "Expecting sys data-type");
    return data.sys;
  }

public:

  // Excluding sys/user-specific data, expected ~24 bytes

  // Time of the event - all events need a time.
  double time = 0.0;
  // If a duration can be expressed in a single event.
  // (Currently only for user-events.. could elim explicit end events.)
  double end_time = 0.0;

  TraceConstantsType type = TraceConstantsType::InvalidTraceType;
  TraceEntryIDType ep = no_trace_entry_id;
  TraceEventIDType event = no_trace_event;
  // If the event relates to a DIFFERENT node.
  NodeType node = uninitialized_destination;

private:

  // Union of sys/user data
  /*union*/ Data data;
};

}} //end namespace vt::trace

#endif /*INCLUDED_TRACE_TRACE_LOG_H*/
