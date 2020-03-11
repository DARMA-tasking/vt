#include <type_traits>

struct TypeX {};

template <typename U, typename = void>
struct has_own_templated_member_t : std::false_type {};

template <typename U>
struct has_own_templated_member_t<U,
  typename std::enable_if<
    std::is_same<
      void (U::*)(TypeX&),
      decltype(&U::template templated_member<TypeX&>)
    >::value,
    void
  >::type
>: std::true_type {};

struct TypeWithTemplatedMember {
  template <typename T>
  void templated_member(T& x) {
    // GCC triggers the inner static_assert with decltype(&T::template<U>).
    // Clang does not, even though an actual invocation without decltype would.
    static_assert(
      std::is_same<T, TypeX>::value and not std::is_same<T, TypeX>::value,
      "static_assert in templated method not expected in decltype(..) usage."
    );
  }
};

int main () {
  static_assert(
    has_own_templated_member_t<TypeWithTemplatedMember>::value,
    "Templated member should be detected."
  );

  return 0;
}
