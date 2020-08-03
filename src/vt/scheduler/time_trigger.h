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

#include <chrono>
#include <queue>
#include <unordered_set>

namespace vt { namespace sched {

/**
 * \struct TimeTrigger
 *
 * \brief A time-based trigger
 */
struct TimeTrigger {
  /**
   * \internal \brief Create a new time-based trigger
   *
   * \param[in] in_period the period to trigger
   * \param[in] in_trigger the action
   * \param[in] in_id the id for the trigger
   */
  TimeTrigger(
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
   * \return the next time this should be triggered
   */
  TimeType nextTriggerTime() const {
    return (last_trigger_time_*1000. + period_.count())/1000.;
  }

  /**
   * \brief Run the trigger
   */
  void runAction() {
    trigger_();
    last_trigger_time_ = timing::Timing::getCurrentTime();
  }

  /**
   * \brief Get the ID for the trigger
   *
   * \return the ID
   */
  int getID() const { return id_; }

  friend bool operator<(TimeTrigger const& lhs, TimeTrigger const& rhs) {
    return lhs.nextTriggerTime() > rhs.nextTriggerTime();
  }

private:
  std::chrono::milliseconds period_;   /**< The trigger's period */
  ActionType trigger_ = nullptr;       /**< The action to trigger  */
  TimeType last_trigger_time_ = 0.;    /**< The last time it was triggered */
  int id_ = -1;                        /**< The trigger's id */
};

/**
 * \struct TimeTriggerList
 *
 * \internal \brief A time-based set of triggers with a given period
 */
struct TimeTriggerList {

  using QueueType = std::priority_queue<TimeTrigger, std::vector<TimeTrigger>>;

  TimeTriggerList() = default;

  /**
   * \brief Add a new trigger
   *
   * \param[in] period time period to trigger action
   * \param[in] action action to trigger
   *
   * \return id to trigger for removal
   */
  int addTrigger(std::chrono::milliseconds period, ActionType action) {
    auto const cur_id = next_trigger_id_++;
    TimeTrigger trigger{period, action, cur_id};
    trigger.runAction();
    queue_.push(trigger);
    return cur_id;
  }

  /**
   * \brief Remove a trigger
   *
   * \param[in] id the \c id to remove
   */
  void removeTrigger(int id) {
    removed_.insert(id);
  }

  /**
   * \brief Trigger any read time-based triggers
   *
   * \param[in] cur_time the current time
   */
  void triggerReady(TimeType cur_time) {
    while (not queue_.empty()) {
      auto iter = removed_.find(queue_.top().getID());
      if (iter != removed_.end()) {
        queue_.pop();
        removed_.erase(iter);
        continue;
      }

      if (queue_.top().nextTriggerTime() < cur_time) {
        auto t = queue_.top();
        queue_.pop();
        t.runAction();
        queue_.push(t);
      } else {
        // all other triggers will not be ready if this one isn't
        break;
      }
    }
  }

private:
  QueueType queue_;                     /**< Priority queue of time triggers */
  int next_trigger_id_ = 0;             /**< Next trigger id */
  std::unordered_set<int> removed_;     /**< Set of delayed removed triggers  */
};

}} /* end namespace vt::sched */

#endif /*INCLUDED_VT_SCHEDULER_TIME_TRIGGER_H*/
