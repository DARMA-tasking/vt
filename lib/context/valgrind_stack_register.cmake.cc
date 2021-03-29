
#include <cstdlib>

#include <valgrind/valgrind.h>

int main(int argc, char** argv) {
  void* stack = std::malloc(16384);
  char* end = static_cast<char*>(stack) + 16384;

  unsigned stack_id = VALGRIND_STACK_REGISTER(stack, end);
  VALGRIND_STACK_DEREGISTER(stack_id);

  std::free(stack);
  return 0;
}
