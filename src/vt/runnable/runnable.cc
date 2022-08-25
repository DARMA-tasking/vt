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
#include "vt/pool/static_sized/memory_pool_equal.h"

namespace vt { namespace runnable {

void RunnableNew::setupHandler(HandlerType handler, bool is_void, TagType tag) {
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
        auto typed_func = theRegistry()->getHandler(handler, tag);
        task_ = [=] { typed_func(msg_.get()); };
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
#if vt_check_enabled(trace_enabled)
  if (ctx_trace_)
    ctx_trace_->begin();
#endif
  if (ctx_continuation_)
    ctx_continuation_->begin();
  if (ctx_lbdata_)
    ctx_lbdata_->begin();
  if (ctx_setcontext_)
    ctx_setcontext_->begin();
  if (ctx_td_)
    ctx_td_->begin();
  if (ctx_collection_)
    ctx_collection_->begin();
}

void RunnableNew::end() {
#if vt_check_enabled(trace_enabled)
  if (ctx_trace_)
    ctx_trace_->end();
#endif
  if (ctx_continuation_)
    ctx_continuation_->end();
  if (ctx_lbdata_)
    ctx_lbdata_->end();
  if (ctx_setcontext_)
    ctx_setcontext_->end();
  if (ctx_td_)
    ctx_td_->end();
  if (ctx_collection_)
    ctx_collection_->end();
}

void RunnableNew::suspend() {
#if vt_check_enabled(trace_enabled)
  if (ctx_trace_)
    ctx_trace_->suspend();
#endif
  if (ctx_continuation_)
    ctx_continuation_->suspend();
  if (ctx_lbdata_)
    ctx_lbdata_->suspend();
  if (ctx_setcontext_)
    ctx_setcontext_->suspend();
  if (ctx_td_)
    ctx_td_->suspend();
  if (ctx_collection_)
    ctx_collection_->suspend();
}

void RunnableNew::resume() {
#if vt_check_enabled(trace_enabled)
  if (ctx_trace_)
    ctx_trace_->resume();
#endif
  if (ctx_continuation_)
    ctx_continuation_->resume();
  if (ctx_lbdata_)
    ctx_lbdata_->resume();
  if (ctx_setcontext_)
    ctx_setcontext_->resume();
  if (ctx_td_)
    ctx_td_->resume();
  if (ctx_collection_)
    ctx_collection_->resume();
}

void RunnableNew::send(elm::ElementIDStruct elm, MsgSizeType bytes) {
#if vt_check_enabled(trace_enabled)
  if (ctx_trace_)
    ctx_trace_->send(elm, bytes);
#endif
  if (ctx_continuation_)
    ctx_continuation_->send(elm, bytes);
  if (ctx_lbdata_)
    ctx_lbdata_->send(elm, bytes);
  if (ctx_setcontext_)
    ctx_setcontext_->send(elm, bytes);
  if (ctx_td_)
    ctx_td_->send(elm, bytes);
  if (ctx_collection_)
    ctx_collection_->send(elm, bytes);
}

/*static*/ std::unique_ptr<
  pool::MemoryPoolEqual<detail::runnable_context_max_size>
> RunnableNew::up_pool =
  std::make_unique<pool::MemoryPoolEqual<detail::runnable_context_max_size>>();

#if vt_check_enabled(trace_enabled)
void RunnableNew::addContext(CtxTracePtr&& ptr) {
  ctx_trace_ = std::move(ptr);
}
#endif

void RunnableNew::addContext(CtxContinuationPtr&& ptr) {
  ctx_continuation_ = std::move(ptr);
}

void RunnableNew::addContext(CtxLBDataPtr&& ptr) {
  ctx_lbdata_ = std::move(ptr);
}

void RunnableNew::addContext(CtxSetContextPtr&& ptr) {
  ctx_setcontext_ = std::move(ptr);
}

void RunnableNew::addContext(CtxTDPtr&& ptr) {
  ctx_td_ = std::move(ptr);
}

void RunnableNew::addContext(CtxCollectionPtr&& ptr) {
  ctx_collection_ = std::move(ptr);
}

#if vt_check_enabled(trace_enabled)
template <>
ctx::Trace* RunnableNew::get<ctx::Trace>() {
  return ctx_trace_.get();
}
#endif

template <>
ctx::Continuation* RunnableNew::get<ctx::Continuation>() {
  return ctx_continuation_.get();
}

template <>
ctx::LBData* RunnableNew::get<ctx::LBData>() {
  return ctx_lbdata_.get();
}

template <>
ctx::SetContext* RunnableNew::get<ctx::SetContext>() {
  return ctx_setcontext_.get();
}

template <>
ctx::TD* RunnableNew::get<ctx::TD>() {
  return ctx_td_.get();
}

template <>
ctx::Collection* RunnableNew::get<ctx::Collection>() {
  return ctx_collection_.get();
}

}} /* end namespace vt::runnable */
