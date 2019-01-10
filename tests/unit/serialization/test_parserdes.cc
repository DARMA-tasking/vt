/*
//@HEADER
// ************************************************************************
//
//                          test_parserdes.cc
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <tuple>
#include <type_traits>

#include "test_parallel_harness.h"
#include "data_message.h"

#include "vt/transport.h"

#define PRINT_DEBUG 0

#if backend_check_enabled(parserdes)

namespace serdes {
template <typename Serializer>
void parserdes(Serializer& s, std::string& str) {
  typename std::string::size_type str_size = str.length();
  s | str_size;
  str.resize(str_size);
  for (auto&& elm : str) {
    s | elm;
  }
}
} /* end namespace serdes */

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

namespace test_data {
struct A : ::vt::Message {
  int32_t a,b,c;
  std::vector<int32_t> v;
  // A() = default;
  A(int32_t a,int32_t b,int32_t c,std::vector<int32_t> v)
    : ::vt::Message(),a(a),b(b),c(c),v(v)
  {}
  /*
   *  This should not be required for parserdes:
   */
  // template <typename Serializer>
  // void serialize(Serializer& s) {
  //   #if PRINT_DEBUG
  //   ::fmt::print(
  //     "test_data::A serialize v.size()={}, elm={}\n",
  //     v.size(), (v.size() == 1 ? v[0] : -1)
  //   );
  //   #endif
  // }
  template <typename Serializer>
  void parserdes(Serializer& s) {
    s & v;
    #if PRINT_DEBUG
    ::fmt::print(
      "test_data::A parserdes v.size()={}, elm={}\n",
      v.size(), (v.size() == 1 ? v[0] : -1)
    );
    #endif
  }
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
  // B() = default;
  B(std::string str_) : ::vt::Message(),str(str_) {}
  /*
   *  This should not be required for parserdes:
   */
  // template <typename Serializer>
  // void serialize(Serializer& s) {
  //   #if PRINT_DEBUG
  //     ::fmt::print("test_data::B serialize str={}\n", str);
  //   #endif
  // }
  template <typename Serializer>
  void parserdes(Serializer& s) {
    s & str;
    #if PRINT_DEBUG
      ::fmt::print("test_data::B parserdes str={}\n", str);
    #endif
  }
  friend std::ostream& operator<<(std::ostream&os, B const& d) {
    return os << "\"" << d.str << "\"";
  }
  friend bool operator==(B const& c1,B const& c2) {
    return c1.str == c2.str;
  }
};
struct C : ::vt::Message {
  std::string str;
  int64_t a;
  // C() = default;
  C(std::string str,int64_t a) : ::vt::Message(),str(str),a(a) {}
  /*
   *  This should not be required for parserdes:
   */
  // template <typename Serializer>
  // void serialize(Serializer& s) {}
  template <typename Serializer>
  void parserdes(Serializer& s) {
    s & str;
  }
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
  static void testHandlerA(MsgT* msg) {
    #if PRINT_DEBUG
      ::fmt::print("testHandlerA: val={}\n", *msg);
    #endif
    EXPECT_EQ(*msg, magic_A_t_);
  }
  template <typename MsgT>
  static void testHandlerB(MsgT* msg) {
    #if PRINT_DEBUG
      ::fmt::print("testHandlerB: val={}\n", *msg);
    #endif
    EXPECT_EQ(*msg, magic_B_t_);
  }
  template <typename MsgT>
  static void testHandlerC(MsgT* msg) {
    #if PRINT_DEBUG
      ::fmt::print("testHandlerC: val={}\n", *msg);
    #endif
    EXPECT_EQ(*msg, magic_C_t_);
  }
};

TEST_F(TestParserdes, test_send_parserdes_a_1) {
  auto const& my_node = theContext()->getNode();

  if (theContext()->getNumNodes() > 1) {
    if (my_node == 0) {
      auto msg = makeSharedMessageSz<test_data::A>(sizeof(int32_t), magic_A_t_);
      #if PRINT_DEBUG
        ::fmt::print("test_data::A sending msg val={}\n", *msg);
      #endif
      SerializedMessenger::sendParserdesMsg<test_data::A, testHandlerA>(1, msg);
    }
  }
}

TEST_F(TestParserdes, test_send_parserdes_b_1) {
  auto const& my_node = theContext()->getNode();

  if (theContext()->getNumNodes() > 1) {
    if (my_node == 0) {
      auto msg = makeSharedMessageSz<test_data::B>(128, magic_B_t_);
      #if PRINT_DEBUG
        ::fmt::print("test_data::B sending msg val={}\n", *msg);
      #endif
      SerializedMessenger::sendParserdesMsg<test_data::B, testHandlerB>(1, msg);
    }
  }
}

TEST_F(TestParserdes, test_send_parserdes_c_1) {
  auto const& my_node = theContext()->getNode();

  if (theContext()->getNumNodes() > 1) {
    if (my_node == 0) {
      auto msg = makeSharedMessageSz<test_data::C>(8*sizeof(char), magic_C_t_);
      #if PRINT_DEBUG
        ::fmt::print("test_data::C sending msg val={}\n", *msg);
      #endif
      SerializedMessenger::sendParserdesMsg<test_data::C, testHandlerC>(1, msg);
    }
  }
}

TEST_F(TestParserdes, test_broadcast_parserdes_a_1) {
  auto const& my_node = theContext()->getNode();

  if (theContext()->getNumNodes() > 1) {
    if (my_node == 0) {
      auto msg = makeSharedMessageSz<test_data::A>(sizeof(int32_t), magic_A_t_);
      #if PRINT_DEBUG
        ::fmt::print("test_data::A broadcasting msg val={}\n", *msg);
      #endif
      SerializedMessenger::broadcastParserdesMsg<test_data::A,testHandlerA>(msg);
    }
  }
}

TEST_F(TestParserdes, test_broadcast_parserdes_b_1) {
  auto const& my_node = theContext()->getNode();

  if (theContext()->getNumNodes() > 1) {
    if (my_node == 0) {
      auto msg = makeSharedMessageSz<test_data::B>(128, magic_B_t_);
      #if PRINT_DEBUG
        ::fmt::print("test_data::B broadcasting msg val={}\n", *msg);
      #endif
      SerializedMessenger::broadcastParserdesMsg<test_data::B,testHandlerB>(msg);
    }
  }
}

TEST_F(TestParserdes, test_broadcast_parserdes_c_1) {
  auto const& my_node = theContext()->getNode();

  if (theContext()->getNumNodes() > 1) {
    if (my_node == 0) {
      auto msg = makeSharedMessageSz<test_data::C>(8*sizeof(char), magic_C_t_);
      #if PRINT_DEBUG
        ::fmt::print("test_data::C broadcasting msg val={}\n", *msg);
      #endif
      SerializedMessenger::broadcastParserdesMsg<test_data::C,testHandlerC>(msg);
    }
  }
}

}}} // end namespace vt::tests::unit

#endif /*backend_check_enabled(parserdes)*/
