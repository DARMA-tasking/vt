
#include "fcontext.h"
#include "util.h"

#include <cstdio>

using namespace fcontext::examples;

static fcontext_t ctx1;
static fcontext_t ctx2;

static void fn1(fcontext_transfer_t t) {
  puts("fn1 1");
  sleep(100);
  auto t1 = jump_fcontext(t.ctx, nullptr);
  puts("fn1 2");
  sleep(100);
  jump_fcontext(t1.ctx, nullptr);
}

static void fn2(fcontext_transfer_t t) {
  puts("fn2 1");
  sleep(100);
  auto t1 = jump_fcontext(t.ctx, nullptr);
  puts("fn2 2");
  sleep(100);
  jump_fcontext(t1.ctx, nullptr);
}

int main(int argc, char** argv) {
  fcontext_stack_t s1 = create_fcontext_stack();
  fcontext_stack_t s2 = create_fcontext_stack();

  ctx1 = make_fcontext_stack(s1, fn1);
  ctx2 = make_fcontext_stack(s2, fn2);

  puts("main 1");
  auto t1 = jump_fcontext(ctx1, nullptr);
  puts("main 2");
  auto t2 = jump_fcontext(ctx2, nullptr);
  puts("main 3");
  auto t3 = jump_fcontext(t1.ctx, nullptr);
  puts("main 4");
  auto t4 = jump_fcontext(t2.ctx, nullptr);
  puts("END");

  destroy_fcontext_stack(s1);
  destroy_fcontext_stack(s2);

  return 0;
}
