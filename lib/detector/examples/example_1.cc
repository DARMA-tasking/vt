
#include "detector.h"

namespace detector { namespace example {

template <typename T>
struct TestTrait {
  template <typename U>
  using DefaultConstructorType = decltype(U());
  using HasDefaultConstructor = detection::is_detected<DefaultConstructorType, T>;

  static constexpr auto const followsTestTrait = HasDefaultConstructor::value;
};

struct Pass {
  Pass() = default;
};

struct Fail {
  Fail() = delete;
};

}} // end namespace detector::example

int main(int, char**) {
  using namespace detector::example;

  static_assert(TestTrait<Pass>::followsTestTrait, "Should follow.");
  static_assert(not TestTrait<Fail>::followsTestTrait, "Should not follow.");

  return 0;
}
