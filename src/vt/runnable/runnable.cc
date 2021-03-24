/*
//@HEADER
// *****************************************************************************
//
//                                 runnable.cc
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

#include "vt/runnable/runnable.h"
#include "vt/objgroup/manager.h"
#include "vt/messaging/envelope.h"
#include "vt/vrt/context/context_vrt.h"
#include "vt/vrt/collection/types/untyped.h"
#include "vt/context/runnable_context/set_context.h"
#include "vt/configs/debug/debug_var_unused.h"

namespace vt { namespace runnable {

void RunnableNew::setupHandler(
  HandlerType handler, bool is_void, TagType tag
) {
  using HandlerManagerType = HandlerManager;
  bool is_obj = HandlerManagerType::isHandlerObjGroup(handler);

  if (not is_void) {
    if (is_obj) {
      task_ = [=]{ objgroup::dispatchObjGroup(msg_, handler); };
    } else {
      auto_registry::NumArgsType num_args = 1;
      bool is_auto = HandlerManagerType::isHandlerAuto(handler);
      bool is_functor = HandlerManagerType::isHandlerFunctor(handler);
      ActiveFnPtrType func;

      if (is_auto && is_functor) {
        func = auto_registry::getAutoHandlerFunctor(handler);
        num_args = auto_registry::getAutoHandlerFunctorArgs(handler);
      } else if (is_auto) {
        func = auto_registry::getAutoHandler(handler);
      } else {
        auto typed_func = theRegistry()->getHandler(handler, tag);
        task_ = [=]{ typed_func(msg_.get()); };
        return;
      }

      if (num_args == 0) {
        auto no_arg_fn = reinterpret_cast<FnParamType<>>(func);
        task_ = [=]{ no_arg_fn(); };
      } else {
        task_ = [=]{ func(msg_.get()); };
      }
    }
  } else {
    bool is_auto = HandlerManagerType::isHandlerAuto(handler);
    bool is_functor = HandlerManagerType::isHandlerFunctor(handler);

    ActiveFnPtrType func = nullptr;

    if (is_auto && is_functor) {
      func = auto_registry::getAutoHandlerFunctor(handler);
    } else if (is_auto) {
      func = auto_registry::getAutoHandler(handler);
    } else {
      vtAbort("Must be auto/functor for a void handler");
    }

    auto void_fn = reinterpret_cast<FnParamType<>>(func);
    task_ = [=] { void_fn(); };
  }
}

void RunnableNew::setupHandlerElement(
  vrt::collection::UntypedCollection* elm, HandlerType handler
) {
  auto const member = HandlerManager::isHandlerMember(handler);
  if (member) {
    auto const func = auto_registry::getAutoHandlerCollectionMem(handler);
    task_ = [=]{ (elm->*func)(msg_.get()); };
  } else {
    auto const func = auto_registry::getAutoHandlerCollection(handler);
    task_ = [=]{ func(msg_.get(), elm); };
  };
}

void RunnableNew::setupHandlerElement(
  vrt::VirtualContext* elm, HandlerType handler
) {
  auto const func = auto_registry::getAutoHandlerVC(handler);
  task_ = [=]{ func(msg_.get(), elm); };
}

void RunnableNew::run() {
  vtAbortIf(
    done_ and not suspended_,
    "Runnable task must either be not done (finished execution) or suspended"
  );

  vt_debug_print(
    context, node,
    "start running task={}, done={}, suspended={}\n",
    print_ptr(this), done_, suspended_
  );

  if (suspended_) {
    resume();
  } else {
    begin();
  }

  vtAssert(task_ != nullptr, "Must have a valid task to run");

#if vt_check_enabled(fcontext)
  if (is_threaded_) {
    using TM = sched::ThreadManager;

    if (suspended_) {
      // find the thread and resume it
      TM::getThread(tid_)->resume();
    } else {
      // allocate a new thread to run the task
      tid_ = TM::allocateThreadRun(task_);
    }

    // check if it is done running, and save that state
    done_ = TM::getThread(tid_)->isDone();

    // cleanup/deallocate the thread
    if (done_) {
      TM::deallocateThread(tid_);
    }
  } else
#endif
  {
    // force use this for when fcontext is disabled to avoid compiler warning
    vt_force_use(is_threaded_, tid_)
    task_();
    done_ = true;
  }

  if (done_) {
    end();
  } else {
    suspended_ = true;
    suspend();
  }

  vt_debug_print(
    context, node,
    "done running task={}, done={}, suspended={}\n",
    print_ptr(this), done_, suspended_
  );
}

void RunnableNew::begin() {
  for (auto&& ctx : contexts_) {
    ctx->begin();
  }
}

void RunnableNew::end() {
  for (auto&& ctx : contexts_) {
    ctx->end();
  }
}

void RunnableNew::suspend() {
  for (auto&& ctx : contexts_) {
    ctx->suspend();
  }
}

void RunnableNew::resume() {
  for (auto&& ctx : contexts_) {
    ctx->resume();
  }
}

void RunnableNew::send(NodeType dest, MsgSizeType size, bool bcast) {
  for (auto&& ctx : contexts_) {
    ctx->send(dest, size, bcast);
  }
}

}} /* end namespace vt::runnable */
