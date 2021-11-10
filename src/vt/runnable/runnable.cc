/*
//@HEADER
// *****************************************************************************
//
//                                 runnable.cc
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

#include "vt/runnable/runnable.h"
#include "vt/objgroup/manager.h"
#include "vt/messaging/envelope.h"
#include "vt/vrt/context/context_vrt.h"
#include "vt/vrt/collection/types/untyped.h"
#include "vt/context/runnable_context/set_context.h"
#include "vt/configs/debug/debug_var_unused.h"
#include "vt/configs/arguments/app_config.h"
#include "vt/scheduler/thread_manager.h"

namespace vt { namespace runnable {

void RunnableNew::setupHandler(
  HandlerType handler, bool is_void, TagType tag
) {
  using HandlerManagerType = HandlerManager;
  bool is_obj = HandlerManagerType::isHandlerObjGroup(handler);

  if (not is_void) {
    if (is_obj) {
      task_ = [=] { objgroup::dispatchObjGroup(msg_, handler); };
      return;
    } else {
      bool const is_auto = HandlerManagerType::isHandlerAuto(handler);
      bool const is_functor = HandlerManagerType::isHandlerFunctor(handler);

      if (is_auto && is_functor) {
        auto f = auto_registry::getAutoHandlerFunctor(handler);
        auto const num_args = auto_registry::getAutoHandlerFunctorArgs(handler);
        if (num_args == 0) {
          task_ = [=] { f->dispatch(nullptr, nullptr); };
        } else {
          task_ = [=] { f->dispatch(msg_.get(), nullptr); };
        }

        return;
      } else if (is_auto) {
        auto f = auto_registry::getAutoHandler(handler);
        task_ = [=] { f->dispatch(msg_.get(), nullptr); };
        return;
      } else {
        auto typed_func = theRegistry()->getHandler(handler, tag);
        task_ = [=] { typed_func(msg_.get()); };
        return;
      }
    }
  } else {
    bool const is_auto = HandlerManagerType::isHandlerAuto(handler);
    bool const is_functor = HandlerManagerType::isHandlerFunctor(handler);

    if (is_auto && is_functor) {
      auto f = auto_registry::getAutoHandlerFunctor(handler);
      task_ = [=] { f->dispatch(nullptr, nullptr); };
      return;
    } else if (is_auto) {
      auto f = auto_registry::getAutoHandler(handler);
      task_ = [=] { f->dispatch(nullptr, nullptr); };
      return;
    } else {
      vtAbort("Must be auto/functor for a void handler");
    }
  }
}

void RunnableNew::setupHandlerElement(
  vrt::collection::UntypedCollection* elm, HandlerType handler
) {
  auto const member = HandlerManager::isHandlerMember(handler);
  if (member) {
    auto const f = auto_registry::getAutoHandlerCollectionMem(handler);
    task_ = [=]{ f->dispatch(msg_.get(), elm); };
  } else {
    auto const f = auto_registry::getAutoHandlerCollection(handler);
    task_ = [=]{ f->dispatch(msg_.get(), elm); };
  };
}

void RunnableNew::setupHandlerElement(
  vrt::VirtualContext* elm, HandlerType handler
) {
  auto const f = auto_registry::getAutoHandlerVC(handler);
  task_ = [=]{ f->dispatch(msg_.get(), elm); };
}

void RunnableNew::run() {
  vtAbortIf(
    done_ and not suspended_,
    "Runnable task must either be not done (finished execution) or suspended"
  );

  vt_debug_print(
    terse, context,
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
  if (is_threaded_ and not theConfig()->vt_ult_disable) {
    auto tm = theSched()->getThreadManager();

    if (suspended_) {
      // find the thread and resume it
      tm->getThread(tid_)->resume();
    } else {
      // allocate a new thread to run the task
      tid_ = tm->allocateThreadRun(task_);
    }

    // check if it is done running, and save that state
    done_ = tm->getThread(tid_)->isDone();

    // cleanup/deallocate the thread
    if (done_) {
      tm->deallocateThread(tid_);
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
    terse, context,
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
