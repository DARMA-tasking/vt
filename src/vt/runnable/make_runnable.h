/*
//@HEADER
// *****************************************************************************
//
//                               make_runnable.h
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

#if !defined INCLUDED_VT_RUNNABLE_MAKE_RUNNABLE_H
#define INCLUDED_VT_RUNNABLE_MAKE_RUNNABLE_H

#include "vt/runnable/runnable.h"
#include "vt/context/runnable_context/td.h"
#include "vt/context/runnable_context/trace.h"
#include "vt/context/runnable_context/from_node.h"
#include "vt/context/runnable_context/set_context.h"
#include "vt/context/runnable_context/collection.h"
#include "vt/context/runnable_context/lb_stats.h"
#include "vt/context/runnable_context/continuation.h"
#include "vt/registry/auto/auto_registry_common.h"

namespace vt { namespace runnable {

/**
 * \struct RunnableMaker
 *
 * \brief Convenience builder class for setting up a \c RunnableNew before
 * enqueuing it or running it.
 */
template <typename MsgT>
struct RunnableMaker {
  /**
   * \internal \brief Construct the builder. Shall not be called directly.
   *
   * \param[in] in_impl the runnable
   * \param[in] in_msg the associated message
   * \param[in] in_handler the handler
   * \param[in] in_han_type the type of handler
   * \param[in] in_from_node the from node for the runnable
   */
  RunnableMaker(
    std::unique_ptr<RunnableNew> in_impl, MsgSharedPtr<MsgT> const& in_msg,
    HandlerType in_handler, NodeType in_from_node
  ) : impl_(std::move(in_impl)),
      msg_(in_msg),
      handler_(in_handler),
      is_void_(in_msg == nullptr),
      from_node_(in_from_node)
  { }
  RunnableMaker(RunnableMaker const&) = delete;
  RunnableMaker(RunnableMaker&&) = default;

  ~RunnableMaker() {
    if (impl_ != nullptr and not is_done_) {
      vtAbort("A runnable was created but never executed or enqueued!")
    }
  }

  /**
   * \brief Add a continuation
   *
   * \param[in] cont the continuation
   */
  RunnableMaker&& withContinuation(ActionType cont) {
    impl_->template addContext<ctx::Continuation>(cont);
    return std::move(*this);
  }

  /**
   * \brief Add a termination epoch
   *
   * \param[in] ep the epoch
   * \param[in] is_term whether it's a termination message
   */
  RunnableMaker&& withTDEpoch(EpochType ep, bool is_term = false) {
    if (not is_term) {
      impl_->template addContext<ctx::TD>(ep);
    }
    return std::move(*this);
  }

  /**
   * \brief Add a termination message to extract an epoch
   *
   * \param[in] is_term whether it's a termination message
   */
  RunnableMaker&& withTDEpochFromMsg(bool is_term = false) {
    if (not is_term) {
      impl_->template addContext<ctx::TD>(msg_);
    }
    return std::move(*this);
  }

  /**
   * \brief Add an typed element handler
   *
   * \param[in] elm the element
   */
  template <typename ElmT>
  RunnableMaker&& withElementHandler(ElmT* elm) {
    set_handler_ = true;
    impl_->setupHandlerElement(elm, handler_);
    return std::move(*this);
  }

  /**
   * \brief Add a collection; sets up the handler and collection contexts
   *
   * \param[in] elm the collection element pointer
   */
  template <typename ElmT, typename IdxT = typename ElmT::IndexType>
  RunnableMaker&& withCollection(ElmT* elm) {
    impl_->template addContext<ctx::Collection<IdxT>>(elm);
    set_handler_ = true;

    if (handler_ != uninitialized_handler) {
      // Be careful with type casting here..convert to typeless before
      // reinterpreting the pointer so the compiler does not produce the wrong
      // offset
      auto void_ptr = static_cast<void*>(elm);
      auto ptr = reinterpret_cast<vrt::collection::UntypedCollection*>(void_ptr);
      impl_->setupHandlerElement(ptr, handler_);
    }

    return std::move(*this);
  }

  /**
   * \brief Add LB stats for instrumentation
   *
   * \param[in] elm the element
   * \param[in] msg the associated message (might be different than the already
   * captured one)
   */
  template <typename ElmT, typename MsgU>
  RunnableMaker&& withLBStats(ElmT* elm, MsgU* msg) {
#if vt_check_enabled(lblite)
    impl_->template addContext<ctx::LBStats>(elm, msg);
#endif
    return std::move(*this);
  }

  /**
   * \brief Add LB stats for instrumentation (without a message)
   *
   * \param[in] elm the element
   */
  template <typename ElmT>
  RunnableMaker&& withLBStatsVoidMsg(ElmT* elm) {
    return withLBStats(&elm->getStats(), elm->getElmID());
  }

  /**
   * \brief Add LB stats for instrumentation directly with element ID and stats
   *
   * \param[in] stats the stats
   * \param[in] elm_id the element ID
   */
  template <typename StatsT, typename T>
  RunnableMaker&& withLBStats(StatsT* stats, T elm_id) {
#if vt_check_enabled(lblite)
    impl_->template addContext<ctx::LBStats>(stats, elm_id);
#endif
    return std::move(*this);
  }

  /**
   * \brief Add LB stats for instrumentation
   *
   * \param[in] elm the element
   */
  template <typename ElmT>
  RunnableMaker&& withLBStats(ElmT* elm) {
#if vt_check_enabled(lblite)
    impl_->template addContext<ctx::LBStats>(elm, msg_.get());
#endif
    return std::move(*this);
  }

  /**
   * \brief Add a trace index (for collection elements)
   *
   * \param[in] trace_event the trace event
   * \param[in] idx1 idx -- dimension 1
   * \param[in] idx2 idx -- dimension 2
   * \param[in] idx3 idx -- dimension 3
   * \param[in] idx4 idx -- dimension 4
   */
  RunnableMaker&& withTraceIndex(
    trace::TraceEventIDType trace_event,
    uint64_t idx1, uint64_t idx2, uint64_t idx3, uint64_t idx4
  ) {
    impl_->template addContext<ctx::Trace>(
      msg_, trace_event, handler_, from_node_, idx1, idx2, idx3, idx4
    );
    return std::move(*this);
  }

  /**
   * \brief Add a tag to the handler
   *
   * \param[in] tag the tag
   */
  RunnableMaker&& withTag(TagType tag) {
    tag_ = tag;
    return std::move(*this);
  }

  /**
   * \brief Run or enqueue the runnable depending on argument
   *
   * \param[in] should_run whether it should be run (if false, it will be
   * enqueued)
   */
  void runOrEnqueue(bool should_run) {
    if (should_run) {
      run();
    } else {
      enqueue();
    }
  }

  /**
   * \brief Run the runnable immediately
   */
  void run() {
    setup();
    impl_->run();
    is_done_ = true;
  }

  /**
   * \brief Enqueue the runnable in the scheduler for execution later
   */
  void enqueue();

  /**
   * \brief Set an explicit task for this runnable (not going through normal
   * handler)
   *
   * \param[in] task_ the task to execute
   */
  RunnableMaker&& withExplicitTask(ActionType task_) {
    impl_->setExplicitTask(task_);
    return std::move(*this);
  }

private:
  /**
   * \internal \brief Setup for running or enqueuing
   */
  void setup() {
    if (not set_handler_) {
      impl_->setupHandler(handler_, is_void_, tag_);
      set_handler_ = true;
    }
  }

private:
  std::unique_ptr<RunnableNew> impl_ = nullptr;
  MsgSharedPtr<MsgT> msg_ = nullptr;
  HandlerType handler_ = uninitialized_handler;
  bool set_handler_ = false;
  TagType tag_ = no_tag;
  bool is_void_ = false;
  NodeType from_node_ = uninitialized_destination;
  bool is_done_ = false;
};

/**
 * \brief Make a new runnable with a message
 *
 * \param[in] msg the message
 * \param[in] is_threaded whether it is threaded
 * \param[in] handler the handler bits
 * \param[in] from the node that caused this runnable to execute
 * \param[in] han_type the type of handler
 *
 * \return the maker for further customization
 */
template <typename U>
RunnableMaker<U> makeRunnable(
  MsgSharedPtr<U> const& msg, bool is_threaded, HandlerType handler, NodeType from
) {
  auto r = std::make_unique<RunnableNew>(msg, is_threaded);
  auto const han_type = HandlerManager::getHandlerRegistryType(handler);
  if (han_type == auto_registry::RegistryTypeEnum::RegVrt or
      han_type == auto_registry::RegistryTypeEnum::RegGeneral or
      han_type == auto_registry::RegistryTypeEnum::RegObjGroup) {
    r->template addContext<ctx::Trace>(msg, handler, from);
  }
  r->template addContext<ctx::FromNode>(from);
  r->template addContext<ctx::SetContext>(r.get());
  return RunnableMaker<U>{std::move(r), msg, handler, from};
}

/**
 * \brief Make a new runnable without a message (void handler)
 *
 * \param[in] is_threaded whether it is threaded
 * \param[in] handler the handler bits
 * \param[in] from the node that caused this runnable to execute
 *
 * \return the maker for further customization
 */
inline RunnableMaker<BaseMsgType> makeRunnableVoid(
  bool is_threaded, HandlerType handler, NodeType from
) {
  // These are currently only types of registry entries that can be void
  auto r = std::make_unique<RunnableNew>(is_threaded);
  // @todo: figure out how to trace this?
  r->template addContext<ctx::FromNode>(from);
  r->template addContext<ctx::SetContext>(r.get());
  return RunnableMaker<BaseMsgType>{std::move(r), nullptr, handler, from};
}

}} /* end namespace vt::runnable */

#include "vt/runnable/make_runnable.impl.h"

#endif /*INCLUDED_VT_RUNNABLE_MAKE_RUNNABLE_H*/
