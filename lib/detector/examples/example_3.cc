
#include "detector.h"

namespace test {

struct MyType {
  MyType() = delete;
  MyType(MyType const&) = delete;
};

}

namespace detector { namespace example {


template <typename T>
struct TestTrait {
  template <typename U>
  using MyTestType = decltype(std::declval<U>().myTest(
    std::declval<int>(), std::declval<double>()
  ));
  using ReturnType = ::test::MyType const&;
  using HasMyTestTypeExact = detection::is_detected_exact<ReturnType, MyTestType, T>;
  using HasMyTestTypeConvert = detection::is_detected_convertible<ReturnType, MyTestType, T>;

  static constexpr auto const followsTestTraitExact = HasMyTestTypeExact::value;
  static constexpr auto const followsTestTraitConvert = HasMyTestTypeConvert::value;
};

struct Pass {
  ::test::MyType const& myTest(int, double);
};

struct Fail1 {
  ::test::MyType& myTest(int, double);
};

struct Fail2 {
  ::test::MyType myTest(int, double);
};

struct Fail3 {
  void myTest(int, double);
};

}} // end namespace detector::example

int main(int, char**) {
  using namespace detector::example;

  static_assert(TestTrait<Pass>::followsTestTraitExact, "Should follow.");
  static_assert(not TestTrait<Fail1>::followsTestTraitExact, "Should not follow.");
  static_assert(not TestTrait<Fail2>::followsTestTraitExact, "Should not follow.");
  static_assert(not TestTrait<Fail3>::followsTestTraitExact, "Should not follow.");


  static_assert(TestTrait<Pass>::followsTestTraitConvert, "Should follow.");
  static_assert(TestTrait<Fail1>::followsTestTraitConvert, "Should follow.");
  static_assert(TestTrait<Fail2>::followsTestTraitConvert, "Should follow.");
  static_assert(not TestTrait<Fail3>::followsTestTraitConvert, "Should not follow.");

  return 0;
}
