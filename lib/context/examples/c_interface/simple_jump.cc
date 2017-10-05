
#include "fcontext.h"
#include "util.h"

#include <cstdio>

using namespace fcontext::examples;

static void fn1(fcontext_transfer_t t) {
  int my_val = 29;
  printf("fn1 1: val=%d\n", my_val);
  sleep(100);
  auto t1 = jump_fcontext(t.ctx, nullptr);
  printf("fn1 2: val=%d\n", my_val);
  sleep(100);
  jump_fcontext(t1.ctx, nullptr);
}

int main(int argc, char** argv) {
  fcontext_stack_t s1 = create_fcontext_stack();
  fcontext_t ctx1 = make_fcontext_stack(s1, fn1);

  puts("main 1");
  auto t = jump_fcontext(ctx1, nullptr);
  puts("main 2");
  jump_fcontext(t.ctx, nullptr);
  puts("END");

  destroy_fcontext_stack(s1);

  return 0;
}
