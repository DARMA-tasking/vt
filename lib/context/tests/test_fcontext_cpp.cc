
#include <time.h>
#include <cstdio>

#include "context_wrapper.h"
#include "stack.h"

using namespace fcontext;

static Context ctx;
static Context ctx2;

inline void sleep(uint32_t _ms) {
  timespec req = { (time_t)_ms / 1000, (long)((_ms % 1000) * 1000000) };
  timespec rem = { 0, 0 };
  nanosleep(&req, &rem);
}

static void bar(fcontext_transfer_t t) {
  puts("BAR");
  sleep(1000);
  jumpContext(t.ctx, NULL);
}

static void foo(fcontext_transfer_t t) {
  puts("FOO");
  sleep(1000);
  jumpContext(ctx2, NULL);
  puts("FOO 2");
  sleep(1000);
  jumpContext(t.ctx, NULL);
}

int main(int argc, char** argv) {
  ContextStackPtr s = fcontext::createStack(16 * 1024);
  ContextStackPtr s2 = createStack();

  ctx = makeContext(s->stack.sptr, s->stack.ssize, foo);
  ctx2 = makeContext(s2->stack.sptr, s2->stack.ssize, bar);

  jumpContext(ctx, NULL);
  puts("END");

  return 0;
}
