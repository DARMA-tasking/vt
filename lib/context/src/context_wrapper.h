
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

struct ContextStack {
  fcontext_stack_t stack;

  ContextStack(fcontext_stack_t in_stack)
    : stack(in_stack)
  { }

  ContextStack(void* in_ptr, size_t in_size) {
    stack.sptr = in_ptr;
    stack.ssize = in_size;
  }
};

using ContextStackPtr = std::unique_ptr<ContextStack>;

// using ContextCallbackFn = void (*)(FContextTransfer);
// using ContextTransferFn = FContextTransfer(*)(FContextTransfer);

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
