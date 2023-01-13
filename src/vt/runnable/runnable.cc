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

void RunnableNew::setupHandler(HandlerType handler, bool is_void) {
  using HandlerManagerType = HandlerManager;
  bool const is_obj = HandlerManagerType::isHandlerObjGroup(handler);

  if (not is_void) {
    if (is_obj) {
      task_ = [=] { objgroup::dispatchObjGroup(msg_, handler); };
      return;
    } else {
      bool const is_auto = HandlerManagerType::isHandlerAuto(handler);
      bool const is_functor = HandlerManagerType::isHandlerFunctor(handler);

      if (is_auto && is_functor) {
        auto const& func = auto_registry::getAutoHandlerFunctor(handler);
        auto const num_args = auto_registry::getAutoHandlerFunctorArgs(handler);
        if (num_args == 0) {
          task_ = [=, &func] { func->dispatch(nullptr, nullptr); };
        } else {
          task_ = [=, &func] { func->dispatch(msg_.get(), nullptr); };
        }

        return;
      } else {
        bool const is_base_msg_derived =
          HandlerManagerType::isHandlerBaseMsgDerived(handler);
        if (is_base_msg_derived) {
          auto const& func = auto_registry::getAutoHandler(handler);
          task_ = [=, &func] { func->dispatch(msg_.get(), nullptr); };
          return;
        }

        auto const& func = auto_registry::getScatterAutoHandler(handler);
        task_ = [=, &func] { func->dispatch(msg_.get(), nullptr); };
        return;
      }
    }
  } else {
    bool const is_auto = HandlerManagerType::isHandlerAuto(handler);
    bool const is_functor = HandlerManagerType::isHandlerFunctor(handler);

    if (is_auto && is_functor) {
      auto const& func = auto_registry::getAutoHandlerFunctor(handler);
      task_ = [=, &func] { func->dispatch(nullptr, nullptr); };
      return;
    } else if (is_auto) {
      bool const is_base_msg_derived =
        HandlerManagerType::isHandlerBaseMsgDerived(handler);
      if (is_base_msg_derived) {
        auto const& func = auto_registry::getAutoHandler(handler);
        task_ = [=, &func] { func->dispatch(msg_.get(), nullptr); };
        return;
      }

      auto const& func = auto_registry::getScatterAutoHandler(handler);
      task_ = [=, &func] { func->dispatch(msg_.get(), nullptr); };
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
  auto const& func = member ?
    auto_registry::getAutoHandlerCollectionMem(handler) :
    auto_registry::getAutoHandlerCollection(handler);
  task_ = [=, &func] { func->dispatch(msg_.get(), elm); };
}

void RunnableNew::setupHandlerElement(
  vrt::VirtualContext* elm, HandlerType handler
) {
  auto const& func = auto_registry::getAutoHandlerVC(handler);
  task_ = [=, &func] { func->dispatch(msg_.get(), elm); };
}

void RunnableNew::run() {
#if vt_check_enabled(fcontext)
  vtAbortIf(
    done_ and not suspended_,
    "Runnable task must either be not done (finished execution) or suspended"
  );

  vt_debug_print(
    terse, context,
    "start running task={}, done={}, suspended={}\n",
    print_ptr(this), done_, suspended_
  );
#endif

#if vt_check_enabled(fcontext)
  if (suspended_) {
    resume();
  } else {
    start();
  }
#else
  start();
#endif

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
#if vt_check_enabled(fcontext)
    // force use this for when fcontext is disabled to avoid compiler warning
    vt_force_use(is_threaded_, tid_)
#endif

    task_();

#if vt_check_enabled(fcontext)
    done_ = true;
#endif
  }

#if vt_check_enabled(fcontext)
  if (done_) {
    finish();
  } else {
    suspended_ = true;
    suspend();
  }
#else
  finish();
#endif

#if vt_check_enabled(fcontext)
  vt_debug_print(
    terse, context,
    "done running task={}, done={}, suspended={}\n",
    print_ptr(this), done_, suspended_
  );
#endif
}

void RunnableNew::start() {
  contexts_.setcontext.start();
  if (contexts_.has_td) contexts_.td.start();
  if (contexts_.has_col) contexts_.col.start();
  if (contexts_.has_lb) contexts_.lb.start();
#if vt_check_enabled(trace_enabled)
  if (contexts_.has_trace) contexts_.trace.start();
#endif
}

void RunnableNew::finish() {
  contexts_.setcontext.finish();
  if (contexts_.has_td) contexts_.td.finish();
  if (contexts_.has_col) contexts_.col.finish();
  if (contexts_.has_cont) contexts_.cont.finish();
  if (contexts_.has_lb) contexts_.lb.finish();
#if vt_check_enabled(trace_enabled)
  if (contexts_.has_trace) contexts_.trace.finish();
#endif
}

void RunnableNew::suspend() {
#if vt_check_enabled(fcontext)
  contexts_.setcontext.suspend();
  if (contexts_.has_td) contexts_.td.suspend();
  if (contexts_.has_col) contexts_.col.suspend();
  if (contexts_.has_lb) contexts_.lb.suspend();

# if vt_check_enabled(trace_enabled)
    if (contexts_.has_trace) contexts_.trace.suspend();
# endif
#endif
}

void RunnableNew::resume() {
#if vt_check_enabled(fcontext)
  contexts_.setcontext.resume();
  if (contexts_.has_td) contexts_.td.resume();
  if (contexts_.has_col) contexts_.col.resume();
  if (contexts_.has_lb) contexts_.lb.resume();

# if vt_check_enabled(trace_enabled)
    if (contexts_.has_trace) contexts_.trace.resume();
# endif
#endif
}

void RunnableNew::send(elm::ElementIDStruct elm, MsgSizeType bytes) {
  if (contexts_.has_lb) contexts_.lb.send(elm, bytes);
}

/*static*/ void* RunnableNew::operator new(std::size_t sz) {
  return RunnableNewAlloc::runnable->alloc(sz,0);
}

/*static*/ void RunnableNew::operator delete(void* ptr) {
  RunnableNewAlloc::runnable->dealloc(ptr);
}

/*static*/
std::unique_ptr<pool::MemoryPoolEqual<sizeof(RunnableNew)>>
RunnableNewAlloc::runnable = std::make_unique<
  pool::MemoryPoolEqual<sizeof(RunnableNew)>
>(256);

}} /* end namespace vt::runnable */

#include "vt/pool/static_sized/memory_pool_equal.impl.h"
