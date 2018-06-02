
#if !defined INCLUDED_COLLECTION_TEST_COLLECTION_COMMON_H
#define INCLUDED_COLLECTION_TEST_COLLECTION_COMMON_H

#include "transport.h"

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <cstdint>
#include <tuple>
#include <string>

namespace serdes {
template <typename Serializer>
void serialize(Serializer& s, std::string& str) {
  typename std::string::size_type str_size = str.length();
  s | str_size;
  str.resize(str_size);
  for (auto&& elm : str) {
    s | elm;
  }
}
} /* end namespace serdes */

namespace test_data {
struct A {
  int32_t a,b,c;
  template <typename Serializer>
  void serialize(Serializer& s) { s | a | b | c; }
  friend std::ostream& operator<<(std::ostream&os, A const& d) {
    return os << d.a << "-" << d.b << "-" << d.c;
  }
  friend bool operator==(A const& c1,A const& c2) {
    return c1.a == c2.a && c1.b == c2.b && c1.c == c2.c;
  }
};
struct B {
  std::string str;
  template <typename Serializer>
  void serialize(Serializer& s) { s | str; }
  friend std::ostream& operator<<(std::ostream&os, B const& d) {
    return os << d.str;
  }
  friend bool operator==(B const& c1,B const& c2) {
    return c1.str == c2.str;
  }
};
struct C {
  std::string str;
  int64_t a;
  template <typename Serializer>
  void serialize(Serializer& s) { s | str | a; }
  friend std::ostream& operator<<(std::ostream&os, C const& d) {
    return os << d.str << '-' << d.a;
  }
  friend bool operator==(C const& c1,C const& c2) {
    return c1.str == c2.str && c1.a == c2.a;
  }
};
} /* end namespace test_data */

namespace vt { namespace tests { namespace unit {

using TestIndex = ::vt::index::Index1D;
using ParamType = int32_t;

static char         magic_string_[]  = "test_string123";
static int32_t      magic_int32_t_   = 29;
static int64_t      magic_int64_t_   = 0xFFFFFEEDFFFFFAAD;
static test_data::A magic_A_t_       = test_data::A{28,2773,12};
static test_data::B magic_B_t_       = test_data::B{"the-ultimate-test"};
static test_data::C magic_C_t_       = test_data::C{"test123",34};

template <typename Tuple, typename=void>
struct ConstructTuple {
  static Tuple construct() { return Tuple{}; }
  static void print(Tuple t) { }
  static void isCorrect(Tuple t) { }
};

#define CONSTRUCT_TUPLE_TYPE(PARAM_TYPE,PARAM_NAME)                     \
  template <typename Tuple>                                             \
  struct ConstructTuple<                                                \
    Tuple,                                                              \
    typename std::enable_if_t<                                          \
      std::is_same<Tuple,std::tuple<PARAM_TYPE>>::value                 \
    >                                                                   \
  >{                                                                    \
    static Tuple construct() {                                          \
      return std::make_tuple(PARAM_NAME);                               \
    }                                                                   \
    static void print(Tuple t) {                                        \
      ::fmt::print(                                                     \
        "{}: val={}\n",theContext()->getNode(),std::get<0>(t)           \
      );                                                                \
    }                                                                   \
    static void isCorrect(Tuple t) {                                    \
      EXPECT_EQ(PARAM_NAME, std::get<0>(t));                            \
    }                                                                   \
};

CONSTRUCT_TUPLE_TYPE(std::string,magic_string_);
CONSTRUCT_TUPLE_TYPE(int32_t,magic_int32_t_);
CONSTRUCT_TUPLE_TYPE(int64_t,magic_int64_t_);
CONSTRUCT_TUPLE_TYPE(test_data::A,magic_A_t_);
CONSTRUCT_TUPLE_TYPE(test_data::B,magic_B_t_);
CONSTRUCT_TUPLE_TYPE(test_data::C,magic_C_t_);

}}} // end namespace vt::tests::unit

#endif /*INCLUDED_COLLECTION_TEST_COLLECTION_COMMON_H*/
