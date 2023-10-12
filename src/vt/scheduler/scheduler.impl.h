/*
//@HEADER
// *****************************************************************************
//
//                               scheduler.impl.h
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

#if !defined INCLUDED_VT_SCHEDULER_SCHEDULER_IMPL_H
#define INCLUDED_VT_SCHEDULER_SCHEDULER_IMPL_H

#include "vt/config.h"
#include "vt/messaging/active.h"
#include "vt/termination/termination.h"

namespace vt {

template <typename Callable>
void runInEpoch(EpochType ep, Callable&& fn) {
  theSched()->triggerEvent(sched::SchedulerEvent::PendingSchedulerLoop);

  theMsg()->pushEpoch(ep);
  fn();
  theMsg()->popEpoch(ep);
  theTerm()->finishedEpoch(ep);
  runSchedulerThrough(ep);
}

template <typename Callable>
void runInEpochCollective(Callable&& fn) {
  runInEpochCollective("UNLABELED", std::forward<Callable>(fn));
}

template <typename Callable>
void runInEpochCollective(std::string const& label, Callable&& fn) {
  auto ep = theTerm()->makeEpochCollective(label);
  runInEpoch(ep, std::forward<Callable>(fn));
}

template <typename Callable>
void runInEpochRooted(Callable&& fn) {
  runInEpochRooted("UNLABELED", std::forward<Callable>(fn));
}

template <typename Callable>
void runInEpochRooted(std::string const& label, Callable&& fn) {
  auto ep = theTerm()->makeEpochRooted(label);
  runInEpoch(ep, std::forward<Callable>(fn));
}

} /* end namespace vt */

namespace vt { namespace sched {

template <typename RunT>
void Scheduler::enqueue(bool is_term, RunT r) {
  if (is_term) {
    num_term_msgs_++;
  }
  auto ep = r->getEpoch();
  if (
    ep != no_epoch and ep != term::any_epoch_sentinel and
    not theTerm()->epochReleased(ep))
  {
    pending_work_[ep].push(UnitType(is_term, r));
  } else {
    work_queue_.emplace(UnitType(is_term, r));
  }
}

template <typename MsgT, typename RunT>
void Scheduler::enqueue(MsgT* msg, RunT r) {
  bool const is_term = envelopeIsTerm(msg->env);

  if (is_term) {
    num_term_msgs_++;
  }

  auto ep = r->getEpoch();
  if (
    ep != no_epoch and ep != term::any_epoch_sentinel and
    not theTerm()->epochReleased(ep))
  {
#   if vt_check_enabled(priorities)
    auto priority = envelopeGetPriority(msg->env);
    pending_work_[ep].push(UnitType(is_term, std::move(r), priority));
#   else
    pending_work_[ep].push(UnitType(is_term, std::move(r)));
#   endif
  } else {
#   if vt_check_enabled(priorities)
    auto priority = envelopeGetPriority(msg->env);
    work_queue_.emplace(UnitType(is_term, std::move(r), priority));
#   else
    work_queue_.emplace(UnitType(is_term, std::move(r)));
#   endif
  }
}

template <typename MsgT, typename RunT>
void Scheduler::enqueue(MsgSharedPtr<MsgT> const& msg, RunT r) {
# if vt_check_enabled(priorities)
  //
  // Assume that MsgSharedPtr<MsgT> is already captured in the action.
  //
  // To speed this up, in the future, we could have a pure message queue that
  // could be dispatched directly based on type/state-bits
  //
  enqueue<MsgT>(msg.get(), std::move(r));
#else
  bool const is_term = envelopeIsTerm(msg->env);
  enqueue<RunT>(is_term, r);
#endif
}

template <typename Callable>
void Scheduler::enqueueLambda(Callable&& c) {
  bool const is_term = false;
# if vt_check_enabled(priorities)
  work_queue_.emplace(
    UnitType(is_term, std::forward<Callable>(c), default_priority)
  );
# else
  work_queue_.emplace(UnitType(is_term, std::forward<Callable>(c)));
# endif
}

template <typename Callable>
void Scheduler::enqueueLambda(PriorityType priority, Callable&& c) {
  bool const is_term = false;
# if vt_check_enabled(priorities)
  work_queue_.emplace(
    UnitType(is_term, std::forward<Callable>(c), priority)
  );
# else
  work_queue_.emplace(UnitType(is_term, std::forward<Callable>(c)));
# endif
}

template <typename RunT>
void Scheduler::enqueue(RunT r) {
  bool const is_term = false;

  auto ep = r->getEpoch();
  if (
    ep != no_epoch and ep != term::any_epoch_sentinel and
    not theTerm()->epochReleased(ep))
  {
#   if vt_check_enabled(priorities)
    pending_work_[ep].push(UnitType(is_term, std::move(r), default_priority));
#   else
    pending_work_[ep].push(UnitType(is_term, std::move(r)));
#   endif
  } else {
#   if vt_check_enabled(priorities)
    work_queue_.emplace(UnitType(is_term, std::move(r), default_priority));
#   else
    work_queue_.emplace(UnitType(is_term, std::move(r)));
# endif
  }
}

template <typename RunT>
void Scheduler::enqueue(PriorityType priority, RunT r) {
  bool const is_term = false;

  auto ep = r->getEpoch();
  if (
    ep != no_epoch and ep != term::any_epoch_sentinel and
    not theTerm()->epochReleased(ep))
  {
#   if vt_check_enabled(priorities)
    pending_work_[ep].push(UnitType(is_term, std::move(r), priority));
#   else
    pending_work_[ep].push(UnitType(is_term, std::move(r)));
#   endif
  } else {
#   if vt_check_enabled(priorities)
    work_queue_.emplace(UnitType(is_term, std::move(r), priority));
#   else
    work_queue_.emplace(UnitType(is_term, std::move(r)));
# endif
  }
}

}} /* end namespace vt::sched */

#endif /*INCLUDED_VT_SCHEDULER_SCHEDULER_IMPL_H*/
