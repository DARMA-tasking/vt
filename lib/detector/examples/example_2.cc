
#include "detector.h"

namespace detector { namespace example {

template <typename T>
struct TestTrait {
  template <typename U>
  using MyTestType = decltype(std::declval<U>().myTest(
    std::declval<int>(), std::declval<double>()
  ));
  using HasMyTestType = detection::is_detected<MyTestType, T>;

  static constexpr auto const followsTestTrait = HasMyTestType::value;
};

struct Pass {
  void myTest(int, double) { }
};

struct Fail {
  void myTest(int) { }
  void myTest(double) { }
  void myTest(int, int) { }
  void myTest(double, int) { }
  void myTest(double, double) { }
};

}} // end namespace detector::example

int main(int, char**) {
  using namespace detector::example;

  static_assert(TestTrait<Pass>::followsTestTrait, "Should follow.");
  static_assert(not TestTrait<Fail>::followsTestTrait, "Should not follow.");

  return 0;
}
