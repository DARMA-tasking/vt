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

namespace vt { namespace trace {

struct Log {
  using LogPtrType         = std::shared_ptr<Log>;
  using TraceConstantsType = eTraceConstants;
  using UserDataType       = int32_t;

  double time = 0.0;
  double end_time = 0.0;

  TraceEntryIDType ep = no_trace_entry_id;
  TraceConstantsType type = TraceConstantsType::InvalidTraceType;
  TraceEventIDType event = no_trace_event;
  TraceMsgLenType msg_len = 0;
  NodeType node = uninitialized_destination;
  uint64_t idx1 = 0, idx2 = 0, idx3 = 0, idx4 = 0;

  std::string user_supplied_note = "";
  UserDataType user_supplied_data = 0;
  UserEventIDType user_event = 0;
  bool user_start = false;

  void setUserNote(std::string const& note) {
    user_supplied_note = note;
  }

  void setUserData(UserDataType data) {
    user_supplied_data = data;
  }

  Log(
    double const in_begin_time, double const in_end_time,
    TraceConstantsType const in_type, std::string const& in_note,
    TraceEventIDType const in_event
  ) : time(in_begin_time), end_time(in_end_time),
      type(in_type), event(in_event), user_supplied_note(in_note)
  { }

  Log(
    double const in_time, TraceConstantsType const in_type,
    std::string const& in_note
  ) : time(in_time), type(in_type), user_supplied_note(in_note)
  { }

  Log(double const in_time, TraceConstantsType const in_type)
    : time(in_time), type(in_type)
  { }

  Log(
    double const in_time, TraceConstantsType const in_type,
    UserEventIDType in_user_event, bool in_user_start
  ) : time(in_time), type(in_type), user_event(in_user_event),
      user_start(in_user_start)
  { }

  Log(
    double const in_time, TraceConstantsType const in_type,
    UserDataType const in_data
  ) : time(in_time), type(in_type), user_supplied_data(in_data)
  { }

  Log(
    double const in_time, TraceEntryIDType const in_ep,
    TraceConstantsType const in_type, TraceMsgLenType const in_msg_len = 0
  ) : time(in_time), ep(in_ep), type(in_type), msg_len(in_msg_len)
  { }

  Log(
    double in_time, TraceEntryIDType in_ep, TraceConstantsType in_type,
    TraceEventIDType in_event, TraceMsgLenType in_msg_len, NodeType in_node,
    uint64_t in_idx1, uint64_t in_idx2, uint64_t in_idx3, uint64_t in_idx4
  ) : time(in_time), ep(in_ep), type(in_type), event(in_event),
      msg_len(in_msg_len), node(in_node), idx1(in_idx1), idx2(in_idx2),
      idx3(in_idx3), idx4(in_idx4)
  { }
};

}} //end namespace vt::trace

#endif /*INCLUDED_TRACE_TRACE_LOG_H*/
