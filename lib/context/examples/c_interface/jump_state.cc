
#include "fcontext.h"
#include "util.h"

#include <cstdio>
#include <cassert>

using namespace fcontext::examples;

static fcontext_t ctx1;
static int* data_ptr = nullptr;

static void fn1(fcontext_transfer_t t) {
  printf("fn1: t.data=%p\n", t.data);

  assert(t.data == data_ptr);

  puts("fn1 1");
  sleep(100);
  auto t1 = jump_fcontext(t.ctx, nullptr);
  puts("fn1 2");
  sleep(100);
  jump_fcontext(t1.ctx, nullptr);
}

int main(int argc, char** argv) {
  fcontext_stack_t s1 = create_fcontext_stack();

  ctx1 = make_fcontext_stack(s1, fn1);

  int data = 10;
  data_ptr = &data;

  printf("main 1: data=%p\n", &data);
  jump_fcontext(ctx1, &data);
  puts("END");

  destroy_fcontext_stack(s1);

  return 0;
}
