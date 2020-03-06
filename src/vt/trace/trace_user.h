/*
//@HEADER
// *****************************************************************************
//
//                                 trace_user.h
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

#if !defined INCLUDED_VT_TRACE_TRACE_USER_H
#define INCLUDED_VT_TRACE_TRACE_USER_H

#include "vt/config.h"
#include "vt/trace/trace_common.h"

#include <string>

namespace vt { namespace trace {

UserEventIDType registerEventCollective(std::string const& name);
UserEventIDType registerEventRooted(std::string const& name);
UserEventIDType registerEventHashed(std::string const& name);

void addUserEvent(UserEventIDType event);
void addUserEventBracketed(UserEventIDType event, double begin, double end);
void addUserNote(std::string const& note);
void addUserData(int32_t data);
void addUserBracketedNote(
  double const begin, double const end, std::string const& note,
  TraceEventIDType const event = no_trace_event
);

void addUserNotePre(std::string const& note, TraceEventIDType const event);
void addUserNoteEpi(std::string const& note, TraceEventIDType const event);

#if backend_check_enabled(trace_enabled)

struct TraceScopedEventHash final {
  /**
   * \begin A scoped (RAII) user event that records duration on destruction.
   *
   * A No-OP scoped event can be created by supplying an empty string.
   */
  explicit TraceScopedEventHash(std::string const& in_str)
    : begin_(Trace::getCurrentTime()),
      str_(in_str)
  {
    event_ = not in_str.empty() ? registerEventHashed(str_) : no_user_event;
  }

  ~TraceScopedEventHash() { end(); }

  void end() {
    if (event_ not_eq no_user_event) {
      end_ = Trace::getCurrentTime();
      theTrace()->addUserEventBracketed(event_, begin_, end_);
      event_ = no_user_event;
    }
  }

private:
  double begin_          = 0.0;
  double end_            = 0.0;
  std::string str_       = "";
  UserEventIDType event_ = 0;
};

struct TraceScopedEvent final {
  /**
   * \begin A scoped (RAII) user event that records duration on destruction.
   *
   * A No-OP scoped event can be created by supplying no_user_event.
   */
  explicit TraceScopedEvent(UserEventIDType event)
    : begin_(Trace::getCurrentTime()),
      event_(event)
  { }

  ~TraceScopedEvent() { end(); }

  void end() {
    if (event_ not_eq no_user_event) {
      end_ = Trace::getCurrentTime();
      theTrace()->addUserEventBracketed(event_, begin_, end_);
      event_ = no_user_event;
    }
  }

private:
  double begin_          = 0.0;
  double end_            = 0.0;
  UserEventIDType event_ = 0;
};

struct TraceScopedNote final {
  TraceScopedNote(
    std::string const& in_note, TraceEventIDType const in_event = no_trace_event
  ) : begin_(Trace::getCurrentTime()),
      event_(in_event),
      note_(in_note)
  { }

  ~TraceScopedNote() {
    end_ = Trace::getCurrentTime();
    theTrace()->addUserBracketedNote(begin_, end_, note_, event_);
  }

  void end() {
    end_ = Trace::getCurrentTime();
  }

  void setEvent(TraceEventIDType const in_event) {
    event_ = in_event;
  }

private:
  double begin_           = 0.0;
  double end_             = 0.0;
  TraceEventIDType event_ = no_trace_event;
  std::string note_       = "";
};

#else

struct TraceScopedNote final {
  TraceScopedNote(std::string const&, TraceEventIDType const = no_trace_event) { }

  void end() { }
  void setEvent(TraceEventIDType const in_event) { }
};

struct TraceScopedEvent final {
  TraceScopedEvent(UserEventIDType) { }

  void end() { }
};

struct TraceScopedEventHash final {
  TraceScopedEventHash(std::string const&) { }

  void end() { }
};

#endif

}} /* end namespace vt::trace */

#endif /*INCLUDED_VT_TRACE_TRACE_USER_H*/
