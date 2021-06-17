/*
//@HEADER
// *****************************************************************************
//
//                              context_wrapper.h
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

#if !defined INCLUDED_CONTEXT_SRC_CONTEXT_WRAPPER_H
#define INCLUDED_CONTEXT_SRC_CONTEXT_WRAPPER_H

#include <cstdlib>
#include <memory>

#include "context/fcontext.h"

namespace fcontext {

struct Context {
  fcontext_t ctx = nullptr;

  Context(fcontext_t const& in_ctx)
    : ctx(in_ctx)
  { }

  Context() = default;
};

using FContext = void*;

struct ContextTransfer {
  fcontext_transfer_t transfer;
};

struct ULTContext {
  bool page_alloced = false;
  fcontext_stack_t stack;

  ULTContext(fcontext_stack_t in_stack, bool const in_page_alloced)
    : page_alloced(in_page_alloced), stack(in_stack)
  { }

  ULTContext(void* in_ptr, size_t in_size, bool const in_page_alloced)
    : page_alloced(in_page_alloced), stack{in_ptr, in_size}
  { }

  ULTContext(ULTContext const&) = default;
  ULTContext(ULTContext&&) = default;
};

using ContextFunctionParam = pfn_fcontext;
using ContextFuncTransfer = fcontext_transfer_t;
using ContextTransferFn = void (*)(ContextTransfer);

inline ContextTransfer jumpContext(Context const to, void* vp = nullptr) {
  return ContextTransfer{jump_fcontext(to.ctx, vp)};
}

// inline ContextTransfer jumpContext(Context const to) {
//   return ContextTransfer{jump_fcontext(to.ctx, nullptr)};
// }

inline ContextTransfer jumpContext(fcontext_transfer_t const to) {
  return ContextTransfer{jump_fcontext(to.ctx, nullptr)};
}

// inline ContextTransfer jumpContext(ContextTransfer to) {
//   return ContextTransfer{jump_fcontext(to.transfer.ctx, nullptr)};
// }

inline Context makeContext(void* sp, size_t size, pfn_fcontext callback) {
  return Context{make_fcontext(sp, size, callback)};
}

inline Context makeContext(ULTContext ctx, pfn_fcontext callback) {
  return Context{make_fcontext(ctx.stack.sptr, ctx.stack.ssize, callback)};
}

// inline Context makeContext(ULTContext ctx, ContextTransferFn fn) {
//   return Context{make_fcontext(
//     ctx.stack.sptr, ctx.stack.ssize, reinterpret_cast<pfn_fcontext>(fn)
//   )};
// }

inline ContextTransfer pushContext(Context const to, void* vp, tfn_fcontext fn) {
  return ContextTransfer{ontop_fcontext(to.ctx, vp, fn)};
}

} /* end namespace fcontext */

#endif /*INCLUDED_CONTEXT_SRC_CONTEXT_WRAPPER_H_*/
