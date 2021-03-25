/*
//@HEADER
// *****************************************************************************
//
//                               thread_action.h
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

#if !defined INCLUDED_VT_SCHEDULER_THREAD_ACTION_H
#define INCLUDED_VT_SCHEDULER_THREAD_ACTION_H

#include "vt/config.h"

#if vt_check_enabled(fcontext)

#include <context/fcontext.h>

namespace vt { namespace sched {

/**
 * \struct ThreadAction
 *
 * \brief An action that runs in a user-level thread with fcontext
 */
struct ThreadAction final {

  /**
   * \brief Construct a \c ThreadAction
   *
   * \param[in] in_action the action to run
   * \param[in] stack_size the size of the stack
   */
  explicit ThreadAction(ActionType in_action, std::size_t stack_size = 0);

  /**
   * \brief Construct a \c ThreadAction
   *
   * \param[in] in_id the thread id
   * \param[in] in_action the action to run
   * \param[in] stack_size the size of the stack
   */
  ThreadAction(
    ThreadIDType in_tid, ActionType in_action, std::size_t stack_size = 0
  );

  ThreadAction(ThreadAction&&) = default;
  ThreadAction(ThreadAction const&) = delete;
  ThreadAction& operator=(ThreadAction&&) = default;
  ThreadAction& operator=(ThreadAction const&) = delete;

  ~ThreadAction();

  /**
   * \brief Run the thread and the action
   */
  void run();

  /**
   * \brief Resume the thread where left off
   */
  void resume();

  /**
   * \brief Keep yielding back to thread (resuming after suspension) until the
   * thread finishes the action
   */
  void runUntilDone();

  /**
   * \brief Get the thread ID
   *
   * \note Return \c no_thread_id if not constructed with one
   *
   * \return the thread ID
   */
  ThreadIDType getThreadID() const { return tid_; }

  /**
   * \brief Check if the thread is done executing
   *
   * \return whether it is done
   */
  bool isDone() const { return done_; }

  /**
   * \brief Suspend the current thread's execution
   */
  static void suspend();

  /**
   * \brief Query whether a thread is currently running
   *
   * \return whether a thread is running
   */
  static bool isThreadActive();

  /**
   * \brief Get the active thread ID for the running \c ThreadAction
   *
   * \return the thread ID
   */
  static ThreadIDType getActiveThreadID();

private:
  /**
   * \brief Static run function to pass the fcontext
   *
   * \param[in] t the transfer
   */
  static void runFnImpl(fcontext_transfer_t t);

private:
  static ThreadAction* cur_running_; /**< The current running \c ThreadAction */

  ThreadIDType tid_ = no_thread_id;         /**< the thread ID */
  ActionType action_ = nullptr;             /**< the action to run  */
  fcontext_stack_t stack_;                  /**< the fcontext stack */
  fcontext_t ctx_;                          /**< the fcontext context */
  fcontext_transfer_t transfer_out_;        /**< transfer out of thread */
  fcontext_transfer_t transfer_in_;         /**< transfer into thread */
  bool done_ = false;                       /**< whether the thread is done */
};

}} /* end namespace vt::sched */

#endif /*vt_check_enabled(fcontext)*/
#endif /*INCLUDED_VT_SCHEDULER_THREAD_ACTION_H*/
