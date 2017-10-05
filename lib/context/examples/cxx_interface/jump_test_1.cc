
#include <time.h>
#include <cstdio>

#include "fcontext.h"

static fcontext_t ctx1;
static fcontext_t ctx2;

inline void sleep(uint32_t _ms) {
  timespec req = { (time_t)_ms / 1000, (long)((_ms % 1000) * 1000000) };
  timespec rem = { 0, 0 };
  nanosleep(&req, &rem);
}

static void fn1(fcontext_transfer_t t) {
  puts("fn1");
  sleep(1000);
  jump_fcontext(t.ctx, nullptr);
}

static void fn2(fcontext_transfer_t t) {
  puts("fn2");
  sleep(1000);
  jump_fcontext(ctx2, nullptr);
  puts("fn2 2");
  sleep(1000);
  jump_fcontext(t.ctx, nullptr);
}

int main(int argc, char** argv) {
  fcontext_stack_t s1 = create_fcontext_stack();
  fcontext_stack_t s2 = create_fcontext_stack();

  ctx1 = make_fcontext_stack(s1, foo);
  ctx2 = make_fcontext_stack(s2, bar);

  jump_fcontext(ctx, nullptr);
  puts("END");

  destroy_fcontext_stack(&s);
  destroy_fcontext_stack(&s2);

  return 0;
}
