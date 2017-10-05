
#if !defined __RUNTIME_TRANSPORT_CONTEXT_WRAPPER__
#define __RUNTIME_TRANSPORT_CONTEXT_WRAPPER__

#include <cstdlib>
#include <memory>

#include "fcontext.h"

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

inline ContextTransfer jumpContext(Context const to, void* vp) {
  return ContextTransfer{jump_fcontext(to.ctx, vp)};
}

inline Context makeContext(void* sp, size_t size, pfn_fcontext callback) {
  return Context{make_fcontext(sp, size, callback)};
}

inline ContextTransfer pushContext(Context const to, void* vp, tfn_fcontext fn) {
  return ContextTransfer{ontop_fcontext(to.ctx, vp, fn)};
}

} /* end namespace fcontext */

#endif /*__RUNTIME_TRANSPORT_CONTEXT_WRAPPER___*/
