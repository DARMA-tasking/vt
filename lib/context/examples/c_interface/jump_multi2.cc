
#include "fcontext.h"
#include "util.h"

#include <cstdio>

using namespace fcontext::examples;

static fcontext_t ctx_root;
static fcontext_t ctx1;
static fcontext_t ctx2;

static void fn_root(fcontext_transfer_t t) {
  puts("fn_root 1");
  sleep(100);
  auto t1 = jump_fcontext(ctx1);
  puts("fn_root 2");
  sleep(100);
  auto t2 = jump_fcontext(ctx2);
  puts("fn_root 3");
  sleep(100);
  jump_fcontext(t1.ctx);
  puts("fn_root 4");
  sleep(100);
  jump_fcontext(t2.ctx);
  puts("fn_root 5");
}

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
  fcontext_stack_t s_root = create_fcontext_stack();
  fcontext_stack_t s1 = create_fcontext_stack();
  fcontext_stack_t s2 = create_fcontext_stack();

  ctx_root = make_fcontext_stack(s_root, fn_root);
  ctx1 = make_fcontext_stack(s1, fn1);
  ctx2 = make_fcontext_stack(s2, fn2);

  auto t = jump_fcontext(ctx_root, nullptr);
  jump_fcontext(t.ctx, nullptr);

  puts("END");

  destroy_fcontext_stack(s_root);
  destroy_fcontext_stack(s1);
  destroy_fcontext_stack(s2);

  return 0;
}
