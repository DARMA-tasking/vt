
#include <time.h>
#include <cstdio>

#include "context/context_wrapper.h"
#include "context/stack.h"

using namespace fcontext;

static Context ctx1;
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
  ULTContextType s1 = createStack(16 * 1024);
  ULTContextType s2 = createStack(0, true);

  ctx1 = makeContext(s1.stack.sptr, s1.stack.ssize, foo);
  ctx2 = makeContext(s2.stack.sptr, s2.stack.ssize, bar);

  jumpContext(ctx1, NULL);
  puts("END");

  return 0;
}
