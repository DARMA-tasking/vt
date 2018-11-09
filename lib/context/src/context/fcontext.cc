
#include "context/fcontext.h"
#include "context/context_stack.h"

#include <cstdlib>

fcontext_stack_t create_fcontext_stack(size_t size) {
  return fcontext::allocateMallocStackInner(size);
}

void destroy_fcontext_stack(fcontext_stack_t stack) {
  return fcontext::destroyStackInner(stack, false);
}

fcontext_t make_fcontext_stack(fcontext_stack_t stack, pfn_fcontext corofn) {
  return make_fcontext(stack.sptr, stack.ssize, corofn);
}
