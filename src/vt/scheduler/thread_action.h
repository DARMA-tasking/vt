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
#include <context/fcontext.h>

namespace vt { namespace scheduler {

struct ThreadAction {

  explicit ThreadAction(ActionType in_action)
    : action_(in_action),
      stack(create_fcontext_stack())
  { }

  ThreadAction(ThreadAction&&) = default;
  ThreadAction(ThreadAction const&) = delete;
  ThreadAction& operator=(ThreadAction&&) = default;
  ThreadAction& operator=(ThreadAction const&) = delete;

  ~ThreadAction() {
    destroy_fcontext_stack(stack);
  }

  void run() {
    ctx = make_fcontext_stack(stack, runFnImpl);
    transfer_in = jump_fcontext(ctx, static_cast<void*>(this));
  }

  void resume() {
    fmt::print("try resume: done={}\n", done_);
    if (done_) {
      return;
    }
    cur_running_ = this;
    transfer_in = jump_fcontext(transfer_in.ctx, nullptr);
  }

  void runUntilDone() {
    run();
    while (!done_) {
      resume();
    }
  }

  static void runFnImpl(fcontext_transfer_t t) {
    fmt::print("running: runFnImpl\n");
    auto ta = static_cast<ThreadAction*>(t.data);
    if (ta->action_) {
      cur_running_ = ta;
      ta->transfer_out = t;
      ta->action_();
      cur_running_ = nullptr;
    }
    fmt::print("done running: runFnImpl\n");
    ta->done_ = true;
    jump_fcontext(ta->transfer_out.ctx, nullptr);
  }

  static void suspend() {
    if (cur_running_ != nullptr) {
      auto x = cur_running_;
      cur_running_ = nullptr;
      fmt::print("suspend\n");
      x->transfer_out = jump_fcontext(x->transfer_out.ctx, nullptr);
    } else {
      fmt::print("Can not suspend---no thread is running\n");
    }
  }

  static ThreadAction* cur_running_;

private:
  ActionType action_ = nullptr;
  fcontext_stack_t stack;
  fcontext_t ctx;
  fcontext_transfer_t transfer_out, transfer_in;
  bool done_ = false;
};

}} /* end namespace vt::scheduler */

#endif /*INCLUDED_VT_SCHEDULER_THREAD_ACTION_H*/
