/*
//@HEADER
// *****************************************************************************
//
//                                set_context.cc
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

#include "vt/context/context.h"
#include "vt/context/runnable_context/set_context.h"
#include "vt/context/context_attorney.h"

namespace vt { namespace ctx {

void SetContext::start() {
  // we have to handle the ugly handler-inside-handler case.. preserve the
  // previous context (pop) and set the new task (push)
  prev_task_ = theContext()->getTask();
  ContextAttorney::setTask(cur_task_);

  vt_debug_print(
    verbose, context,
    "{}: begin(): prev={}, task={}\n",
    print_ptr(this), print_ptr(prev_task_.get()), print_ptr(cur_task_.get())
  );
}

void SetContext::finish() {
  vt_debug_print(
    verbose, context,
    "{}: end(): prev={}, task={}\n",
    print_ptr(this), print_ptr(prev_task_.get()), print_ptr(theContext()->getTask())
  );

  vtAssert(
    theContext()->getTask() == cur_task_, "Must be correct task"
  );
  ContextAttorney::setTask(prev_task_);
}

void SetContext::suspend() {
  vt_debug_print(
    verbose, context,
    "{}: suspend(): prev={}, task={}\n",
    print_ptr(this), print_ptr(prev_task_.get()), print_ptr(cur_task_.get())
  );

  // Get the innermost task, that actually called for this thread to block
  suspended_task_ = theContext()->getTask();

  vtAssert(prev_task_ == nullptr, "There should be no prev_task");

  ContextAttorney::setTask(prev_task_);
}

void SetContext::resume() {
  vt_debug_print(
    verbose, context,
    "{}: resume(): prev={}, task={}\n",
    print_ptr(this), print_ptr(prev_task_.get()), print_ptr(cur_task_.get())
  );

  prev_task_ = theContext()->getTask();
  ContextAttorney::setTask(suspended_task_);
  suspended_task_ = nullptr;
}

}} /* end namespace vt::ctx */
