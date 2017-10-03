
#include "fcontext.h"

#include <cstdlib>

fcontext_stack_t create_fcontext_stack(size_t size) {
  fcontext_stack_t stack;
  stack.sptr = malloc(size);
  stack.ssize = size;
  return stack;
}

void destroy_fcontext_stack(fcontext_stack_t* s) {
  free(s->sptr);
  return;
}
