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

void RunnableNew::setupHandler(HandlerType handler) {
  using HandlerManagerType = HandlerManager;

  bool const is_obj = HandlerManagerType::isHandlerObjGroup(handler);
  vtAssert(not is_obj, "Must not be object");

  bool const is_auto = HandlerManagerType::isHandlerAuto(handler);
  bool const is_functor = HandlerManagerType::isHandlerFunctor(handler);

  if (is_auto && is_functor) {
    f_.func_ = auto_registry::getAutoHandlerFunctor(handler).get();
    return;
  } else {
    bool const is_base_msg_derived =
      HandlerManagerType::isHandlerBaseMsgDerived(handler);
    if (is_base_msg_derived) {
      f_.func_ = auto_registry::getAutoHandler(handler).get();
      return;
    }

    is_scatter_ = true;
    f_.func_scat_ = auto_registry::getScatterAutoHandler(handler).get();
    return;
  }
}

void RunnableNew::setupHandlerObjGroup(void* obj, HandlerType handler) {
  f_.func_ = auto_registry::getAutoHandlerObjGroup(handler).get();
  obj_ = obj;
}

void RunnableNew::setupHandlerElement(
  vrt::collection::UntypedCollection* elm, HandlerType handler
) {
  auto const member = HandlerManager::isHandlerMember(handler);
  f_.func_ = member ?
    auto_registry::getAutoHandlerCollectionMem(handler).get() :
    auto_registry::getAutoHandlerCollection(handler).get();
  obj_ = elm;
}

void RunnableNew::setupHandlerElement(
  vrt::VirtualContext* elm, HandlerType handler
) {
  f_.func_ = auto_registry::getAutoHandlerVC(handler).get();
  obj_ = elm;
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

  bool needs_time = false;
#if vt_check_enabled(trace_enabled)
  if (contexts_.has_trace) needs_time = true;
  else
#endif
  if (contexts_.has_lb)
  {
    needs_time = contexts_.lb.needsTime();
  }
  TimeType start_time = needs_time ? theSched()->getRecentTime() : NAN;

#if vt_check_enabled(fcontext)
  if (suspended_) {
    resume(start_time);
  } else
#endif
  {
    start(start_time);
  }

#if vt_check_enabled(fcontext)
  if (is_threaded_ and not theConfig()->vt_ult_disable) {
    auto tm = theSched()->getThreadManager();

    if (suspended_) {
      // find the thread and resume it
      tm->getThread(tid_)->resume();
    } else {
      // allocate a new thread to run the task
      tid_ = tm->allocateThreadRun([&]{
        f_.func_->dispatch(msg_ == nullptr ? nullptr : msg_.get(), obj_);
      });
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

    if (is_scatter_) {
      f_.func_scat_->dispatch(msg_ == nullptr ? nullptr : msg_.get(), obj_);
    } else {
      f_.func_->dispatch(msg_ == nullptr ? nullptr : msg_.get(), obj_);
    }

#if vt_check_enabled(fcontext)
    done_ = true;
#endif
  }
  theSched()->setRecentTimeToStale();
  TimeType end_time = needs_time ? theSched()->getRecentTime() : NAN;



#if vt_check_enabled(fcontext)
  if (done_) {
    finish(end_time);
  } else {
    suspended_ = true;
    suspend(end_time);
  }
#else
  finish(end_time);
#endif

#if vt_check_enabled(fcontext)
  vt_debug_print(
    terse, context,
    "done running task={}, done={}, suspended={}\n",
    print_ptr(this), done_, suspended_
  );
#endif
}

void RunnableNew::start(TimeType time) {
  contexts_.setcontext.start();
  if (contexts_.has_td) contexts_.td.start();
  if (contexts_.has_col) contexts_.col.start();
  if (contexts_.has_lb) contexts_.lb.start(time);
#if vt_check_enabled(trace_enabled)
  if (contexts_.has_trace) contexts_.trace.start(time);
#endif
}

void RunnableNew::finish(TimeType time) {
  contexts_.setcontext.finish();
  if (contexts_.has_td) contexts_.td.finish();
  if (contexts_.has_col) contexts_.col.finish();
  if (contexts_.has_cont) contexts_.cont.finish();
  if (contexts_.has_lb) contexts_.lb.finish(time);
#if vt_check_enabled(trace_enabled)
  if (contexts_.has_trace) contexts_.trace.finish(time);
#endif
}

void RunnableNew::suspend(TimeType time) {
#if vt_check_enabled(fcontext)
  contexts_.setcontext.suspend();
  if (contexts_.has_td) contexts_.td.suspend();
  if (contexts_.has_col) contexts_.col.suspend();
  if (contexts_.has_lb) contexts_.lb.suspend(time);

# if vt_check_enabled(trace_enabled)
    if (contexts_.has_trace) contexts_.trace.suspend(time);
# endif
#endif
}

void RunnableNew::resume(TimeType time) {
#if vt_check_enabled(fcontext)
  contexts_.setcontext.resume();
  if (contexts_.has_td) contexts_.td.resume();
  if (contexts_.has_col) contexts_.col.resume();
  if (contexts_.has_lb) contexts_.lb.resume(time);

# if vt_check_enabled(trace_enabled)
    if (contexts_.has_trace) contexts_.trace.resume(time);
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
