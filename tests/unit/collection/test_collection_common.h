/*
//@HEADER
// *****************************************************************************
//
//                           test_collection_common.h
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_COLLECTION_TEST_COLLECTION_COMMON_H
#define INCLUDED_COLLECTION_TEST_COLLECTION_COMMON_H

#include "vt/transport.h"

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <gtest/gtest.h>

#include <cstdint>
#include <tuple>
#include <string>

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

using TestIndex = ::vt::Index1D;
using ParamType = int32_t;

static char         magic_string_[]  =
  "test_string123-long-string-for-testing_"
  "test_string123-long-string-for-testing_"
  "test_string123-long-string-for-testing_"
  "test_string123-long-string-for-testing_"
  "test_string123-long-string-for-testing_"
  "test_string123-long-string-for-testing_";
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

CONSTRUCT_TUPLE_TYPE(std::string,magic_string_)
CONSTRUCT_TUPLE_TYPE(int32_t,magic_int32_t_)
CONSTRUCT_TUPLE_TYPE(int64_t,magic_int64_t_)
CONSTRUCT_TUPLE_TYPE(test_data::A,magic_A_t_)
CONSTRUCT_TUPLE_TYPE(test_data::B,magic_B_t_)
CONSTRUCT_TUPLE_TYPE(test_data::C,magic_C_t_)

}}} // end namespace vt::tests::unit

#endif /*INCLUDED_COLLECTION_TEST_COLLECTION_COMMON_H*/
