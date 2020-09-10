/*
//@HEADER
// *****************************************************************************
//
//                            time_trigger_manager.h
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

#if !defined INCLUDED_VT_TIMETRIGGER_TIME_TRIGGER_MANAGER_H
#define INCLUDED_VT_TIMETRIGGER_TIME_TRIGGER_MANAGER_H

#include "vt/timetrigger/trigger.h"
#include "vt/runtime/component/component.h"

#include <chrono>
#include <queue>
#include <unordered_set>
#include <vector>

namespace vt { namespace timetrigger {

/**
 * \struct TimeTriggerManager
 *
 * \brief A time-based manager of triggers that each have an associated time
 * period that get triggered from the progress function.
 *
 * Timed triggers are "drifting", meaning that they approximate the associated
 * time period but are not absolutely fixed in time.
 */
struct TimeTriggerManager
  : runtime::component::PollableComponent<TimeTriggerManager>
{
  /// A queue prioritized by the earliest next trigger to execute
  using QueueType = std::priority_queue<Trigger, std::vector<Trigger>>;

  TimeTriggerManager() = default;

  std::string name() override { return "TimeTriggerManager"; }

  int progress() override;

  /**
   * \brief Register a time-based trigger with a specific period
   *
   * \param[in] current_time current time
   * \param[in] period time period to trigger action
   * \param[in] action action to trigger
   * \param[in] fire_immediately whether to wait the period before triggering
   * the first time
   *
   * \return the trigger id (can be used for removal)
   */
  int addTrigger(
    TimeType current_time, std::chrono::milliseconds period,
    ActionType action, bool fire_immediately = false
  );

  /**
   * \brief Unregister a time-based trigger
   *
   * \param[in] id the \c id to remove
   */
  void removeTrigger(int id);

  /**
   * \brief Trigger any read time-based triggers
   *
   * \param[in] cur_time the current time
   */
  void triggerReady(TimeType cur_time);

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | queue_
      | next_trigger_id_
      | removed_;
  }

private:
  QueueType queue_;                     /**< Priority queue of time triggers */
  int next_trigger_id_ = 0;             /**< Next trigger id */
  std::unordered_set<int> removed_;     /**< Set of delayed removed triggers  */
};

}} /* end namespace vt::timetrigger */

namespace vt {

extern timetrigger::TimeTriggerManager* theTimeTrigger();

}  //end namespace vt

#endif /*INCLUDED_VT_TIMETRIGGER_TIME_TRIGGER_MANAGER_H*/
