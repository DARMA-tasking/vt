/*
//@HEADER
// *****************************************************************************
//
//                                time_trigger.h
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

#if !defined INCLUDED_VT_SCHEDULER_TIME_TRIGGER_H
#define INCLUDED_VT_SCHEDULER_TIME_TRIGGER_H

#include "vt/timing/timing_type.h"
#include "vt/timing/timing.h"

#include <map>

namespace vt { namespace sched {

/**
 * \struct TimeTrigger
 *
 * \internal \brief A time-based set of triggers with a given period
 */
struct TimeTrigger {
  using Milliseconds = int32_t;

  /**
   * \internal \brief Create a new time-based trigger for a given period
   *
   * \param[in] in_period the period for this trigger set
   */
  explicit TimeTrigger(Milliseconds in_period)
    : period_(in_period)
  { }

  /**
   * \brief Add a new trigger
   *
   * \param[in] action action to trigger
   *
   * \return id to trigger for removal
   */
  int addTrigger(ActionType action) {
    auto cur_trigger = next_trigger_++;
    triggers_[cur_trigger] = action;
    return cur_trigger;
  }

  /**
   * \brief Remove a trigger
   *
   * \param[in] id the \c id to remove
   */
  void removeTrigger(int id) {
    auto iter = triggers_.find(id);
    if (iter != triggers_.end()) {
      triggers_.erase(iter);
    }
  }

  /**
   * \brief Get the number of triggers
   *
   * \return number of registered triggers
   */
  std::size_t numTriggers() const {
    return triggers_.size();
  }

  /**
   * \brief Trigger all actions
   *
   * \param[in] cur_time the current time
   */
  inline void trigger(TimeType cur_time) {
    using timing::Timing;

    last_trigger_ = cur_time;
    for (auto&& elm : triggers_) {
      elm.second();
    }
  }

  /**
   * \brief Test if we should trigger based on period and last trigger time
   *
   * \param[in] cur_time the current time
   *
   * \return whether it should trigger
   */
  inline bool shouldTrigger(TimeType cur_time) const {
    return (cur_time - last_trigger_)*1000 > period_;
  }

  /**
   * \brief Get last trigger time
   *
   * \return last time was triggered
   */
  inline TimeType getLastTriggerTime() const { return last_trigger_; }

private:
  TimeType last_trigger_ = 0.f;          /**< Last time this was triggered */
  int next_trigger_ = 0;                 /**< ID for next trigger */
  Milliseconds period_ = 1000;           /**< Period for this set of triggers */
  std::map<int, ActionType> triggers_;   /**< Actions to trigger keyed by id */
};

}} /* end namespace vt::sched */

#endif /*INCLUDED_VT_SCHEDULER_TIME_TRIGGER_H*/
