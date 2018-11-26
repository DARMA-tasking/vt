/*
//@HEADER
// ************************************************************************
//
//                          seq_ult_context.h
//                                VT
//              Copyright (C) 2017 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_SEQUENCE_SEQ_ULT_CONTEXT_H
#define INCLUDED_SEQUENCE_SEQ_ULT_CONTEXT_H

#include "vt/config.h"
#include "vt/sequence/seq_common.h"

#include <context/context.h>

#include <functional>

namespace vt { namespace seq {

static struct SeqULTConstTag { } seq_ult_cons_tag_t { };

void seq_context_fn(fcontext::ContextFuncTransfer t);

struct SeqULTContext {
  using ULTContextFuncType = fcontext::ContextFunctionParam;
  using ULTContextStatefulFnType = std::function<void()>;

  fcontext::ULTContextType stack;
  fcontext::Context fctx;

  explicit SeqULTContext(SeqULTConstTag);

  SeqULTContext() : SeqULTContext(seq_ult_cons_tag_t) { }

  void initialize(ULTContextFuncType func);
  void initialize(ULTContextStatefulFnType stateful_func);
  bool initialized() const;
  bool isContextActive() const;
  void runStateFunc(fcontext::ContextFuncTransfer* state);
  void setCurTransferState(fcontext::ContextFuncTransfer* state);
  void clearCurTransferState();

  void start();
  void suspend();
  void resume();
  void finish();

private:
  bool has_valid_context_state_ = false;

  fcontext::ContextTransfer transfer_holder_main_;
  fcontext::ContextTransfer transfer_holder_ctx_;

  fcontext::ContextFuncTransfer* cur_transfer_main_state_ = nullptr;

  ULTContextStatefulFnType state_fn_ = nullptr;

  bool context_initialized = false;
};

}} /* end namespace vt::seq */

#endif /*INCLUDED_SEQUENCE_SEQ_ULT_CONTEXT_H*/
