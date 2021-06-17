/*
//@HEADER
// *****************************************************************************
//
//                              seq_ult_context.cc
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

#include "vt/config.h"
#include "vt/sequence/seq_common.h"
#include "vt/sequence/seq_ult_context.h"

#include <cassert>
#include <functional>

namespace vt { namespace seq {

#if vt_check_enabled(fcontext)
void seq_context_fn(fcontext::ContextFuncTransfer t) {
  void* data = t.data;
  SeqULTContext* ctx = reinterpret_cast<SeqULTContext*>(data);
  ctx->runStateFunc(&t);
  ctx->finish();
  fcontext::jumpContext(t);
}
#endif

#if vt_check_enabled(fcontext)
  SeqULTContext::SeqULTContext(SeqULTConstTag)
    : stack(fcontext::createStack())
  { }
#else
  SeqULTContext::SeqULTContext(SeqULTConstTag) {
    vtAbort("fcontext is not enabled, seq_ult_context should not be used");
  }
#endif

#if vt_check_enabled(fcontext)
void SeqULTContext::initialize(ULTContextFuncType func) {
  context_initialized = true;
  fctx = fcontext::makeContext(stack, func);
}

void SeqULTContext::initialize(ULTContextStatefulFnType stateful_func) {
  context_initialized = true;
  fctx = fcontext::makeContext(stack, seq_context_fn);
  state_fn_ = stateful_func;
}

bool SeqULTContext::initialized() const {
  return context_initialized;
}

bool SeqULTContext::isContextActive() const {
  return has_valid_context_state_;
}

void SeqULTContext::runStateFunc(fcontext::ContextFuncTransfer* state) {
  vt_debug_print_force(
    normal, sequence,
    "SeqULTContext: runStateFunc: state={}\n", print_ptr(state)
  );

  vtAssert(state_fn_ != nullptr, "Must have valid state fn");
  setCurTransferState(state);
  state_fn_();
}

void SeqULTContext::setCurTransferState(fcontext::ContextFuncTransfer* state) {
  cur_transfer_main_state_ = state;
}

void SeqULTContext::clearCurTransferState() {
  cur_transfer_main_state_ = nullptr;
}
#endif

void SeqULTContext::start() {
#if vt_check_enabled(fcontext)
  vt_debug_print_force(
    normal, sequence,
    "SeqULTContext: start\n"
  );

  has_valid_context_state_ = true;

  transfer_holder_ctx_ = fcontext::jumpContext(fctx, static_cast<void*>(this));
#endif
}

void SeqULTContext::suspend() {
#if vt_check_enabled(fcontext)
  vtAssert(cur_transfer_main_state_ != nullptr, "Must have valid state");
  has_valid_context_state_ = true;

  vt_debug_print_force(
    normal, sequence,
    "SeqULTContext: suspend: cur_transfer_main_state={}\n",
    print_ptr(cur_transfer_main_state_)
  );

  transfer_holder_main_ = fcontext::jumpContext(cur_transfer_main_state_->ctx);
  cur_transfer_main_state_ = &transfer_holder_main_.transfer;
#endif
}

void SeqULTContext::resume() {
#if vt_check_enabled(fcontext)
  vtAssert(has_valid_context_state_, "Must have valid context state");
  transfer_holder_ctx_ = fcontext::jumpContext(transfer_holder_ctx_.transfer);
#endif
}

void SeqULTContext::finish() {
#if vt_check_enabled(fcontext)
  vtAssert(has_valid_context_state_, "Must have valid context state");
  has_valid_context_state_ = false;
  cur_transfer_main_state_ = nullptr;
#endif
}


}} /* end namespace vt::seq */
