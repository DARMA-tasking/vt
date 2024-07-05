/*
//@HEADER
// *****************************************************************************
//
//                                trace_user.cc
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

#include "vt/config.h"
#include "vt/trace/trace_common.h"
#include "vt/trace/trace.h"
#include "vt/trace/trace_user.h"

#include <string>
#include <unordered_map>

namespace vt { namespace trace {

UserEventIDType registerEventCollective(
  [[maybe_unused]] std::string const& name
) {
#if vt_check_enabled(trace_enabled)
  return theTrace()->registerUserEventColl(name);
#else
  return 0;
#endif
}

UserEventIDType registerEventRooted([[maybe_unused]] std::string const& name) {
#if vt_check_enabled(trace_enabled)
  return theTrace()->registerUserEventRoot(name);
#else
  return 0;
#endif
}

UserEventIDType registerEventHashed([[maybe_unused]] std::string const& name) {
#if vt_check_enabled(trace_enabled)
  return theTrace()->registerUserEventHash(name);
#else
  return 0;
#endif
}

void addUserEvent([[maybe_unused]] UserEventIDType event) {
#if vt_check_enabled(trace_enabled)
  theTrace()->addUserEvent(event);
#endif
}

void addUserEventBracketedBeginTime(UserEventIDType event, TimeType begin) {
#if vt_check_enabled(trace_enabled)
  theTrace()->addUserEventBracketedBeginTime(event, begin);
#endif
}

void addUserEventBracketedEndTime(UserEventIDType event, TimeType end) {
#if vt_check_enabled(trace_enabled)
  theTrace()->addUserEventBracketedEndTime(event, end);
#endif
}

void addUserNote([[maybe_unused]] std::string const& note) {
#if vt_check_enabled(trace_enabled)
  theTrace()->addUserNote(note);
#endif
}

void addUserData([[maybe_unused]] int32_t data) {
#if vt_check_enabled(trace_enabled)
  theTrace()->addUserData(data);
#endif
}

void addUserBracketedNote(
  [[maybe_unused]] TimeType const begin, [[maybe_unused]] TimeType const end,
  [[maybe_unused]] std::string const& note,
  [[maybe_unused]] TraceEventIDType const event
) {
#if vt_check_enabled(trace_enabled)
  theTrace()->addUserBracketedNote(begin, end, note, event);
#endif
}

#if vt_check_enabled(trace_enabled)
struct UserNoteHolder final {
  static std::unordered_map<std::string, TraceScopedNote> notes_;
};

/*static*/ std::unordered_map<std::string, TraceScopedNote> UserNoteHolder::notes_ = {};
#endif

void addUserNotePre(
  [[maybe_unused]] std::string const& in_note,
  [[maybe_unused]] TraceEventIDType const in_event
) {
#if vt_check_enabled(trace_enabled)
  auto iter = UserNoteHolder::notes_.find(in_note);
  vtAssertExpr(iter == UserNoteHolder::notes_.end());
  UserNoteHolder::notes_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(in_note),
    std::forward_as_tuple(in_note, in_event)
  );
#endif
}

void addUserNoteEpi(
  [[maybe_unused]] std::string const& in_note,
  [[maybe_unused]] TraceEventIDType const in_event
) {
#if vt_check_enabled(trace_enabled)
  auto iter = UserNoteHolder::notes_.find(in_note);
  vtAssertExpr(iter != UserNoteHolder::notes_.end());
  UserNoteHolder::notes_.erase(iter);
#endif
}


}} /* end namespace vt::trace */
