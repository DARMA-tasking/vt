
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
    printf("a=%f,b=%d\n",a,b);
    jumpContext(t);
  }
};

struct Fn2 {
  void operator()(fcontext_transfer_t t, int b) {
    printf("b=%d\n",b);
    jumpContext(t);
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
  fcontext::functor::jumpContext(ctx1);
  puts("main 2");
  fcontext::functor::jumpContext(ctx2);
  puts("main END");

  destroyStack(s1);
  destroyStack(s2);

  return 0;
}
