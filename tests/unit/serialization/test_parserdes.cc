
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <tuple>
#include <type_traits>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "transport.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

namespace test_data {
struct A : ::vt::Message {
  int32_t a,b,c;
  std::vector<int32_t> v;
  A() = default;
  A(int32_t a,int32_t b,int32_t c,std::vector<int32_t> v)
    : ::vt::Message(),a(a),b(b),c(c),v(v)
  {}
  template <typename Serializer> void serialize(Serializer& s) {}
  template <typename Serializer>
  void parserdes(Serializer& s) { s & v; }
  friend std::ostream& operator<<(std::ostream&os, A const& d) {
    return os << d.a << "-" << d.b << "-" << d.c
              << "," << (d.v.size() == 1 ? d.v[0] : -1);
  }
  friend bool operator==(A const& c1,A const& c2) {
    return c1.a == c2.a && c1.b == c2.b && c1.c == c2.c && c1.v == c2.v;
  }
};
struct B : ::vt::Message {
  std::string str;
  B() = default;
  B(std::string str) : ::vt::Message(),str(str) {}
  template <typename Serializer> void serialize(Serializer& s) {}
  template <typename Serializer>
  void parserdes(Serializer& s) { s & str; }
  friend std::ostream& operator<<(std::ostream&os, B const& d) {
    return os << d.str;
  }
  friend bool operator==(B const& c1,B const& c2) {
    return c1.str == c2.str;
  }
};
struct C : ::vt::Message {
  std::string str;
  int64_t a;
  C() = default;
  C(std::string str,int64_t a) : ::vt::Message(),str(str),a(a) {}
  template <typename Serializer> void serialize(Serializer& s) {}
  template <typename Serializer>
  void parserdes(Serializer& s) { s & str; }
  friend std::ostream& operator<<(std::ostream&os, C const& d) {
    return os << d.str << '-' << d.a;
  }
  friend bool operator==(C const& c1,C const& c2) {
    return c1.str == c2.str && c1.a == c2.a;
  }
};
} /* end namespace test_data */

static test_data::A magic_A_t_       = test_data::A{28,2773,12,{32}};
static test_data::B magic_B_t_       = test_data::B{"the-ultimate-test"};
static test_data::C magic_C_t_       = test_data::C{"test123",34};

struct TestParserdes : TestParallelHarness {
  template <typename MsgT>
  static void testHandlerB(MsgT* msg) {
    ::fmt::print("testHandlerB: val={}\n", *msg);
    EXPECT_EQ(*msg, magic_B_t_);
  }
};

TEST_F(TestParserdes, test_parserdes_b1) {
  auto const& my_node = theContext()->getNode();

  if (theContext()->getNumNodes() > 1) {
    if (my_node == 0) {
      auto msg = makeSharedMessageSz<test_data::B>(128);
      SerializedMessenger::sendParserdesMsg<test_data::B, testHandlerB>(1, msg);
    }
  }
}

}}} // end namespace vt::tests::unit
