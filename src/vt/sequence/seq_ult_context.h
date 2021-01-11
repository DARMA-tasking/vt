/*
//@HEADER
// *****************************************************************************
//
//                              seq_ult_context.h
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

#if !defined INCLUDED_SEQUENCE_SEQ_ULT_CONTEXT_H
#define INCLUDED_SEQUENCE_SEQ_ULT_CONTEXT_H

#include "vt/config.h"
#include "vt/sequence/seq_common.h"

#if backend_check_enabled(fcontext)
#  include <context/context.h>
#endif

#include <functional>

namespace vt { namespace seq {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static struct SeqULTConstTag { } seq_ult_cons_tag_t { };
#pragma GCC diagnostic pop

#if backend_check_enabled(fcontext)
  void seq_context_fn(fcontext::ContextFuncTransfer t);
#endif

struct SeqULTContext {
#if backend_check_enabled(fcontext)
  using ULTContextFuncType = fcontext::ContextFunctionParam;
#endif

  using ULTContextStatefulFnType = std::function<void()>;

#if backend_check_enabled(fcontext)
  fcontext::ULTContextType stack;
  fcontext::Context fctx;
#endif

  explicit SeqULTContext(SeqULTConstTag);

  SeqULTContext() : SeqULTContext(seq_ult_cons_tag_t) { }

#if backend_check_enabled(fcontext)
  void initialize(ULTContextFuncType func);
  void initialize(ULTContextStatefulFnType stateful_func);
  bool initialized() const;
  bool isContextActive() const;
  void runStateFunc(fcontext::ContextFuncTransfer* state);
  void setCurTransferState(fcontext::ContextFuncTransfer* state);
  void clearCurTransferState();
#endif

  void start();
  void suspend();
  void resume();
  void finish();

private:
#if backend_check_enabled(fcontext)
  bool has_valid_context_state_ = false;

  fcontext::ContextTransfer transfer_holder_main_;
  fcontext::ContextTransfer transfer_holder_ctx_;

  fcontext::ContextFuncTransfer* cur_transfer_main_state_ = nullptr;

  ULTContextStatefulFnType state_fn_ = nullptr;

  bool context_initialized = false;
#endif
};

}} /* end namespace vt::seq */

#endif /*INCLUDED_SEQUENCE_SEQ_ULT_CONTEXT_H*/
