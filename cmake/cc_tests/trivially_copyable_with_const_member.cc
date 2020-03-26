#include <type_traits>

struct StructWithConstMember {
  const int y;

  StructWithConstMember() : y(22) {
  }
};

int main() {
  static_assert(
    std::is_trivially_copyable<StructWithConstMember>::value,
    "Expected to be trivially copyable, even with const members."
  );
  return 0;
}
