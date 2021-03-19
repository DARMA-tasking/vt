/*
//@HEADER
// *****************************************************************************
//
//                                set_context.h
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

#if !defined INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_SET_CONTEXT_H
#define INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_SET_CONTEXT_H

#include "vt/context/runnable_context/base.h"
#include "vt/runnable/runnable.fwd.h"

namespace vt { namespace ctx {

/**
 * \struct SetContext
 *
 * \brief Set the context of the current running task for query by other
 * components or users.
 */
struct SetContext : Base {

  /**
   * \brief Construct a \c SetContext
   *
   * \param[in] in_nonowning_cur_task the current task (non-owning ptr held)
   */
  explicit SetContext(runnable::RunnableNew* in_nonowning_cur_task)
    : nonowning_cur_task_(in_nonowning_cur_task)
  {}

  /**
   * \brief Preserve the existing task and replace with a new one
   */
  void begin() override {
    // we have to handle the ugly handler-inside-handler case.. preserve the
    // previous context (pop) and set the new task (push)
    nonowning_prev_task_ = theContext()->getTask();
    theContext()->setTask(nonowning_cur_task_);
  }

  /**
   * \brief Restore the previous existing task to the context (if there was one)
   */
  void end() override {
    vtAssert(
      theContext()->getTask() == nonowning_cur_task_, "Must be correct task"
    );
    theContext()->setTask(nonowning_prev_task_);
  }

  void suspend() override {
    end();
  }

  void resume() override {
    begin();
  }

private:
  /// The previous runnable that was in the context
  runnable::RunnableNew* nonowning_prev_task_ = nullptr;
  /// The new runnable that is replacing it
  runnable::RunnableNew* nonowning_cur_task_ = nullptr;
};

}} /* end namespace vt::ctx */

#endif /*INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_SET_CONTEXT_H*/
