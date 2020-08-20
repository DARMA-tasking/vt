/*
//@HEADER
// *****************************************************************************
//
//                                  trigger.h
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

#if !defined INCLUDED_VT_TIMETRIGGER_TRIGGER_H
#define INCLUDED_VT_TIMETRIGGER_TRIGGER_H

#include "vt/configs/types/types_type.h"
#include "vt/timing/timing_type.h"
#include "vt/timing/timing.h"

#include <chrono>

namespace vt { namespace timetrigger {

/**
 * \internal \struct Trigger
 *
 * \brief A time-based trigger that fires with some time period
 */
struct Trigger {
  /**
   * \internal \brief Create a new time-based trigger
   *
   * \param[in] in_period the period to trigger
   * \param[in] in_trigger the action
   * \param[in] in_id the id for the trigger
   */
  Trigger(
    std::chrono::milliseconds in_period, ActionType in_trigger, int in_id
  ) : period_(in_period),
      trigger_(in_trigger),
      id_(in_id)
  { }

  /**
   * \brief Get the last trigger time
   *
   * \return the last time this was triggered
   */
  TimeType getLastTriggerTime() const { return last_trigger_time_; }

  /**
   * \brief Get the next trigger time
   *
   * \param[in] current_time current time
   *
   * \return the next time this should be triggered
   */
  TimeType nextTriggerTime() const {
    return (last_trigger_time_*1000.0 + period_.count())/1000.0;
  }

  /**
   * \brief Run the trigger
   *
   * \param[in] current_time current time
   */
  void runAction(TimeType current_time) {
    trigger_();
    last_trigger_time_ = current_time;
  }

  /**
   * \brief Get the ID for the trigger
   *
   * \return the ID
   */
  int getID() const { return id_; }

  /**
   * \brief Check if the trigger is ready to be fired
   *
   * \return whether the trigger can be run already
   */
  bool ready(TimeType current_time) const {
    return nextTriggerTime() < current_time;
  }

  /**
   * \internal \brief Update last trigger time---used the first time to set up
   * the trigger
   *
   * \param[in] trigger_time the last trigger time
   */
  void setLastTriggerTime(TimeType trigger_time) {
    last_trigger_time_ = trigger_time;
  }

  friend bool operator<(Trigger const& lhs, Trigger const& rhs) {
    return lhs.nextTriggerTime() > rhs.nextTriggerTime();
  }

private:
  std::chrono::milliseconds period_;   /**< The trigger's period */
  ActionType trigger_ = nullptr;       /**< The action to trigger  */
  TimeType last_trigger_time_ = 0.;    /**< The last time it was triggered */
  int id_ = -1;                        /**< The trigger's id */
};

}} /* end namespace vt::timetrigger */

#endif /*INCLUDED_VT_TIMETRIGGER_TRIGGER_H*/
