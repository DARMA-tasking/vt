/*
//@HEADER
// *****************************************************************************
//
//                                   trace.h
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

#if !defined INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_TRACE_H
#define INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_TRACE_H

#include "vt/context/runnable_context/base.h"
#include "vt/trace/trace_common.h"
#include "vt/messaging/envelope/envelope_get.h"
#include "vt/registry/auto/auto_registry_common.h"

namespace vt { namespace ctx {

#if vt_check_enabled(trace_enabled)

/**
 * \struct Trace
 *
 * \brief Manages tracing a task's execution for outputting logs
 */
struct Trace final : Base {

  /**
   * \brief Construct a new trace context (basic processing event)
   *
   * \param[in] msg the associated message
   * \param[in] in_handler the handler
   * \param[in] in_from_node the node that instigated this event
   * \param[in] in_han_type the type of handler for tracing
   */
  template <typename MsgT>
  Trace(
    MsgT const& msg, HandlerType const in_handler, NodeType const in_from_node
  );

  /**
   * \brief Construct a new trace context (collection processing event)
   *
   * \param[in] msg the associated message
   * \param[in] in_trace_event the trace event associated with this event
   * \param[in] in_handler the handler
   * \param[in] in_from_node the node that instigated this event
   * \param[in] in_han_type the type of handler for tracing
   * \param[in] in_idx1 1-dimension index
   * \param[in] in_idx2 2-dimension index
   * \param[in] in_idx3 3-dimension index
   * \param[in] in_idx4 4-dimension index
   */
  template <typename MsgT>
  Trace(
    MsgT const& msg, trace::TraceEventIDType const in_trace_event,
    HandlerType const in_handler, NodeType const in_from_node,
    uint64_t in_idx1, uint64_t in_idx2, uint64_t in_idx3, uint64_t in_idx4
  );

  /**
   * \brief Get the current trace event
   *
   * \return the current trace event
   */
  trace::TraceEventIDType getEvent() const { return event_; }

  void begin() final override;
  void end() final override;
  void suspend() final override;
  void resume() final override;

private:
  /// Whether it's a collection or not
  bool is_collection_ = false;
  /// The current trace event
  trace::TraceEventIDType event_ = trace::no_trace_event;
  /// The size of the message for tracing
  std::size_t msg_size_ = 0;
  /// Whether this is traced
  bool is_traced_ = false;
  /// The from node
  NodeType from_node_ = uninitialized_destination;
  /// The active handler for extracting trace info
  HandlerType handler_ = uninitialized_handler;
  /// The collection indices
  uint64_t idx1_ = 0, idx2_ = 0, idx3_ = 0, idx4_ = 0;
  /// The open processing tag
  trace::TraceProcessingTag processing_tag_;
};

#else

struct Trace : Base {

  template <typename... Args>
  Trace(Args&&... args) {}

};

#endif

}} /* end namespace vt::ctx */

#include "vt/context/runnable_context/trace.impl.h"

#endif /*INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_TRACE_H*/
