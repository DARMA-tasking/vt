/*
//@HEADER
// *****************************************************************************
//
//                               thread_action.cc
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

#include "vt/scheduler/thread_action.h"
#include "vt/messaging/active.h"
#include "vt/configs/arguments/app_config.h"

#if vt_check_enabled(fcontext)

namespace vt { namespace sched {

ThreadAction::ThreadAction(ActionType in_action, std::size_t stack_size)
  : ThreadAction(no_thread_id, in_action, stack_size)
{ }

ThreadAction::ThreadAction(
  ThreadIDType in_tid, ActionType in_action, std::size_t stack_size
) : tid_(in_tid),
    action_(in_action),
    stack_(
      create_fcontext_stack(
        stack_size == 0 ? theConfig()->vt_ult_stack_size : stack_size
      )
    )
{ }

ThreadAction::~ThreadAction() {
  destroy_fcontext_stack(stack_);
}

/*static*/ ThreadAction* ThreadAction::cur_running_ = nullptr;

void ThreadAction::run() {
  ctx_ = make_fcontext_stack(stack_, runFnImpl);
  transfer_in_ = jump_fcontext(ctx_, static_cast<void*>(this));
}

void ThreadAction::resume() {
  vt_debug_print(
    gen, node,
    "try resume: isDone={}\n", done_
  );

  if (done_) {
    return;
  }

  cur_running_ = this;
  transfer_in_ = jump_fcontext(transfer_in_.ctx, nullptr);
}

void ThreadAction::runUntilDone() {
  run();
  while (not done_) {
    resume();
  }
}

/*static*/ void ThreadAction::runFnImpl(fcontext_transfer_t t) {
  vt_debug_print(
    gen, node,
    "start running: runFnImpl\n"
  );

  auto ta = static_cast<ThreadAction*>(t.data);
  if (ta->action_) {
    cur_running_ = ta;
    ta->transfer_out_ = t;
    ta->action_();
    cur_running_ = nullptr;
  }

  vt_debug_print(
    gen, node,
    "finished running: runFnImpl\n"
  );

  ta->done_ = true;
  jump_fcontext(ta->transfer_out_.ctx, nullptr);
}

/*static*/ void ThreadAction::suspend() {
  if (cur_running_ != nullptr) {
    auto x = cur_running_;
    cur_running_ = nullptr;
    vt_debug_print(gen, node, "suspend\n");
    x->transfer_out_ = jump_fcontext(x->transfer_out_.ctx, nullptr);
  } else {
    fmt::print("Can not suspend---no thread is running\n");
  }
}

/*static*/ bool ThreadAction::isThreadActive() {
  return cur_running_ != nullptr;
}

/*static*/ ThreadIDType ThreadAction::getActiveThreadID() {
  if (cur_running_) {
    return cur_running_->tid_;
  } else {
    return no_thread_id;
  }
}

}} /* end namespace vt::sched */

#endif /*vt_check_enabled(fcontext)*/
