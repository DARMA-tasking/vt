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

/**
 * \brief Register a user event collectively
 *
 * \param[in] name the name of the event
 *
 * \return the user event ID for logging
 */
UserEventIDType registerEventCollective(std::string const& name);

/**
 * \brief Register a user event (rooted) on a single node
 *
 * \param[in] name the name of the event
 *
 * \return the user event ID for logging
 */
UserEventIDType registerEventRooted(std::string const& name);

/**
 * \brief Idempotent registration of a user event on a single node
 *
 * \warning This call can be dangerous because while it does allow impromptu
 * user event creation, any collisions in the hash will cause multiple events
 * to be conflated to the same event
 *
 * \param[in] name the name of the event
 *
 * \return the user event ID for logging
 */
UserEventIDType registerEventHashed(std::string const& name);

/**
 * \brief Log a user event
 *
 * \param[in] event the event ID to log
 */
void addUserEvent(UserEventIDType event);

/**
 * \brief Log a bracketed user event
 *
 * \note See \c TraceScopedEvent for a safer scope-based logging mechanism for
 * bracketed user events.
 *
 * \param[in] event the event ID to log
 * \param[in] begin the begin time
 * \param[in] end the end time
 */
void addUserEventBracketed(UserEventIDType event, double begin, double end);

/**
 * \brief Log a user note
 *
 * \param[in] note the note to log
 */
void addUserNote(std::string const& note);

/**
 * \brief Log user data as an integer
 *
 * \param[in] data the integer to log
 */
void addUserData(int32_t data);

/**
 * \brief Log a bracketed user event with a note
 *
 * \note See \c TraceScopedNote for a safer scope-based logging mechanism for
 * bracketed user events with a note.
 *
 * \param[in] begin the begin time
 * \param[in] end the end time
 * \param[in] note the note to log
 * \param[in] event the event ID to log
 */
void addUserBracketedNote(
  double const begin, double const end, std::string const& note,
  TraceEventIDType const event = no_trace_event
);

/**
 * \brief Log the start for a bracketed user event with a note
 *
 * \param[in] note the note
 * \param[in] event the pre-registered user event ID
 */
void addUserNotePre(std::string const& note, TraceEventIDType const event);

/**
 * \brief Log the end for a bracketed user event with a note
 *
 * \param[in] note the note
 * \param[in] event the pre-registered user event ID
 */
void addUserNoteEpi(std::string const& note, TraceEventIDType const event);

#if vt_check_enabled(trace_enabled)

/**
 * \struct TraceScopedEventHash
 *
 * \brief A scoped user event using a hash
 *
 * This is safer than manually starting and stopping the logged event because it
 * automatically closes when the scope ends, timing that portion of the program
 * for the logged bracketed region.
 *
 * \warning This call can be dangerous because while it does allow impromptu
 * user event type creation, any collisions in the hash will cause multiple
 * events to be conflated to the same event type.
 */
struct TraceScopedEventHash final {
  /**
   * \brief Construct the hashed event
   *
   * \param[in] in_str the name of the event
   */
  explicit TraceScopedEventHash(std::string const& in_str)
    : begin_(TraceLite::getCurrentTime()),
      str_(in_str)
  {
    event_ = registerEventHashed(str_);
  }

  TraceScopedEventHash& operator=(TraceScopedEventHash const&) = delete;

  ~TraceScopedEventHash() { end(); }

  /**
   * \brief Manually end the scoped event early (before it goes out of scope)
   */
  void end() {
    if (event_ != no_user_event_id) {
      double end = TraceLite::getCurrentTime();
      theTrace()->addUserEventBracketed(event_, begin_, end);

      event_ = no_user_event_id;
    }
  }

private:
  double begin_          = 0.0;
  std::string str_       = "";
  UserEventIDType event_ = no_user_event_id;
};

/**
 * \struct TraceScopedEvent
 *
 * \brief A scoped user event with a pre-registered user event ID
 *
 * This is safer than manually starting and stopping the logged event because it
 * automatically closes when the scope ends, timing that portion of the program
 * for the logged bracketed region.
 */
struct TraceScopedEvent final {
  /**
   * \brief Construct the trace scoped event, recording the start time
   *
   * \param[in] event the user event to log (e.g., may be obtained from
   * \c registerEventCollective )
   */
  explicit TraceScopedEvent(UserEventIDType event)
    : begin_(event != no_user_event_id ? TraceLite::getCurrentTime() : 0),
      event_(event)
  { }

  TraceScopedEvent& operator=(TraceScopedEvent const&) = delete;

  ~TraceScopedEvent() { end(); }

  /**
   * \brief Manually end the scoped event early (before it goes out of scope)
   */
  void end() {
    if (event_ != no_user_event_id) {
      double end = TraceLite::getCurrentTime();
      theTrace()->addUserEventBracketed(event_, begin_, end);

      event_ = no_user_event_id;
    }
  }

private:
  double begin_          = 0.0;
  UserEventIDType event_ = no_user_event_id;
};

/**
 * \struct TraceScopedNote
 *
 * \brief A scoped user event from a pre-registered user event ID with a note
 *
 * This is safer than manually starting and stopping the logged event because it
 * automatically closes when the scope ends, timing that portion of the program
 * for the logged bracketed region.
 */
struct TraceScopedNote final {
  /**
   * \brief Construct a scoped event with a note
   *
   * \param[in] in_note the user note to record
   * \param[in] event the user event to log (e.g., may be obtained from
   * \c registerEventCollective )
   */
  TraceScopedNote(
    std::string const& in_note, TraceEventIDType const in_event = no_trace_event
  ) : begin_(in_event != no_trace_event ? TraceLite::getCurrentTime() : 0),
      event_(in_event),
      note_(in_note)
  { }

  TraceScopedNote& operator=(TraceScopedNote const&) = delete;

  ~TraceScopedNote() { end(); }

  /**
   * \brief Manually end the scoped event early (before it goes out of scope)
   */
  void end() {
    if (event_ != no_user_event_id) {
      double end = TraceLite::getCurrentTime();
      theTrace()->addUserBracketedNote(begin_, end, note_, event_);

      event_ = no_user_event_id;
    }
  }

  void setEvent(TraceEventIDType const in_event) {
    event_ = in_event;
  }

private:
  double begin_           = 0.0;
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
