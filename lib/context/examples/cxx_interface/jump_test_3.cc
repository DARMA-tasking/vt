
#include "context_wrapper.h"
#include "context_functor.h"
#include "fcontext.h"
#include "stack.h"
#include "util.h"

#include <time.h>
#include <cstdio>

using namespace fcontext;
using namespace fcontext::functor;
using namespace fcontext::examples;

struct Fn1 {
  void operator()(fcontext_transfer_t t, double a, int b) {
    printf("fn1: a=%f,b=%d\n",a,b);
    puts("fn1 1");
    sleep(100);
    auto t1 = jumpContext(t);
    puts("fn1 2");
    sleep(100);
    jumpContext(t1.transfer);
  }
};

struct Fn2 {
  void operator()(fcontext_transfer_t t, int b) {
    printf("b=%d\n",b);
    puts("fn2 1");
    sleep(100);
    auto t1 = jumpContext(t);
    puts("fn2 2");
    sleep(100);
    jumpContext(t1.transfer);
  }
};

Context<Fn1, double, int> ctx1;
Context<Fn2, int> ctx2;

int main(int argc, char** argv) {
  ULTContextType s1 = createStack();
  ULTContextType s2 = createStack();

  ctx1 = fcontext::functor::makeContext<Fn1>(s1, 10.0, 20);
  ctx2 = fcontext::functor::makeContext<Fn2>(s2, 30);

  puts("main 1");
  auto t1 = fcontext::functor::jumpContextFunc(ctx1);
  puts("main 2");
  auto t2 = fcontext::functor::jumpContextFunc(ctx2);
  puts("main 3");
  jumpContext(t1);
  puts("main 4");
  jumpContext(t2);
  puts("main END");

  destroyStack(s1);
  destroyStack(s2);

  return 0;
}
