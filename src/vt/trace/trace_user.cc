/*
//@HEADER
// ************************************************************************
//
//                          trace_user.cc
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#include "vt/config.h"
#include "vt/trace/trace_common.h"
#include "vt/trace/trace.h"
#include "vt/trace/trace_user.h"

#include <string>
#include <unordered_map>

namespace vt { namespace trace {

UserEventIDType registerEventCollective(std::string const& name) {
#if backend_check_enabled(trace_enabled)
  return theTrace()->registerUserEventColl(name);
#else
  return 0;
#endif
}

UserEventIDType registerEventRooted(std::string const& name) {
#if backend_check_enabled(trace_enabled)
  return theTrace()->registerUserEventRoot(name);
#else
  return 0;
#endif
}

void addUserEvent(UserEventIDType event) {
#if backend_check_enabled(trace_enabled)
  theTrace()->addUserEvent(event);
#endif
}

void addUserEventBracketed(UserEventIDType event, double begin, double end) {
#if backend_check_enabled(trace_enabled)
  theTrace()->addUserEventBracketed(event, begin, end);
#endif
}

void addUserNote(std::string const& note) {
#if backend_check_enabled(trace_enabled)
  theTrace()->addUserNote(note);
#endif
}

void addUserData(int32_t data) {
#if backend_check_enabled(trace_enabled)
  theTrace()->addUserData(data);
#endif
}

void addUserBracketedNote(
  double const begin, double const end, std::string const& note,
  TraceEventIDType const event
) {
#if backend_check_enabled(trace_enabled)
  theTrace()->addUserBracketedNote(begin, end, note, event);
#endif
}

#if backend_check_enabled(trace_enabled)
struct UserSplitHolder final {
  static std::unordered_map<std::string, double> split_;
};

/*static*/ std::unordered_map<std::string, double> UserSplitHolder::split_ = {};
#endif

void addUserNotePre(std::string const& note, TraceEventIDType const) {
#if backend_check_enabled(trace_enabled)
  auto iter = UserSplitHolder::split_.find(note);
  vtAssertExpr(iter == UserSplitHolder::split_.end());
  UserSplitHolder::split_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(note),
    std::forward_as_tuple(Trace::getCurrentTime())
  );
#endif
}

void addUserNoteEpi(std::string const& in_note, TraceEventIDType const event) {
#if backend_check_enabled(trace_enabled)
  auto iter = UserSplitHolder::split_.find(in_note);
  vtAssertExpr(iter != UserSplitHolder::split_.end());
  auto begin = iter->second;
  auto end = Trace::getCurrentTime();
  theTrace()->addUserBracketedNote(begin, end, in_note, event);
  UserSplitHolder::split_.erase(iter);
#endif
}


}} /* end namespace vt::trace */
