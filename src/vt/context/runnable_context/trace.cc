/*
//@HEADER
// *****************************************************************************
//
//                                   trace.cc
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

#if !defined INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_TRACE_CC
#define INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_TRACE_CC

#include "vt/config.h"

#if vt_check_enabled(trace_enabled)

#include "vt/context/runnable_context/trace.h"
#include "vt/registry/auto/auto_registry_interface.h"
#include "vt/messaging/active.h"

namespace vt { namespace ctx {

void Trace::begin() {
  if (not is_traced_) {
    return;
  }

  auto const trace_id = auto_registry::handlerTraceID(handler_);

  if (is_collection_) {
    auto const cur_node = theContext()->getFromNodeCurrentTask();
    auto const from_node =
      from_node_ != uninitialized_destination ? from_node_ : cur_node;

    processing_tag_ = theTrace()->beginProcessing(
      trace_id, msg_size_, event_, from_node, idx1_, idx2_, idx3_, idx4_
    );
  } else {
    processing_tag_ = theTrace()->beginProcessing(
      trace_id, msg_size_, event_, from_node_
    );
  }
}

void Trace::end() {
  if (not is_traced_) {
    return;
  }

  theTrace()->endProcessing(processing_tag_);
}

void Trace::suspend() {
  end();
}

void Trace::resume() {
  // @todo: connect up the last event to this new one after suspension
  begin();
}

}} /* end namespace vt::ctx */

#endif /*vt_check_enabled(trace_enabled)*/
#endif /*INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_TRACE_CC*/
