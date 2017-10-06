
#include "context_wrapper.h"
#include "fcontext.h"
#include "stack.h"
#include "util.h"

#include <time.h>
#include <cstdio>

using namespace fcontext;
using namespace fcontext::examples;

static Context ctx1;
static Context ctx2;

static void fn1(fcontext_transfer_t t) {
  puts("fn1 1");
  sleep(100);
  jumpContext(t);
  puts("fn1 2");
  sleep(100);
  jumpContext(t);
}

static void fn2(fcontext_transfer_t t) {
  puts("fn2 1");
  sleep(100);
  jumpContext(t);
  puts("fn2 2");
  sleep(100);
  jumpContext(t);
}

int main(int argc, char** argv) {
  ULTContextType s1 = createStack();
  ULTContextType s2 = createStack();

  ctx1 = makeContext(s1, fn1);
  ctx2 = makeContext(s2, fn2);

  puts("main 1");
  auto t1 = jumpContext(ctx1);
  puts("main 2");
  auto t2 = jumpContext(ctx2);
  puts("main 3");
  jumpContext(t1.transfer);
  puts("main 4");
  jumpContext(t2.transfer);
  puts("END");

  destroyStack(s1);
  destroyStack(s2);

  return 0;
}
