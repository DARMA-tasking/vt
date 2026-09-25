/*
//@HEADER
// *****************************************************************************
//
//                        test_demangler_utils.nompi.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#include <gtest/gtest.h>

#include <array>
#include <string>
#include <utility>

#include "vt/utils/demangle/demangle.h"
#include "test_harness.h"

namespace vt { namespace tests { namespace unit {

struct TestDemanglerUtils : TestHarness {
  template <class MyType, MyType PF_VALUE_NAME>
  static std::string getExpectedTypeName() {
    std::string signature = __PRETTY_FUNCTION__;
    auto labelPos = signature.rfind("PF_VALUE_NAME");
    auto typePos = signature.find(" = ", labelPos);
    auto lastBracket = signature.rfind("]");

    assert(typePos != std::string::npos);
    assert(lastBracket != std::string::npos);

    return signature.substr(typePos + 3, lastBracket - (typePos + 3));
  }

  template <class T, T PF_VALUE_NAME>
  static std::string prettyFunctionForValue() {
    return __PRETTY_FUNCTION__;
  }

  template <auto f>
  struct TestFunctor {
    using FuncType = decltype(f);

    static std::string getPrettyName() {
      return prettyFunctionForValue<FuncType, f>();
    }
    std::pair<std::string, std::string> operator()() {
      return {getPrettyName(), getExpectedTypeName<FuncType, f>()};
    }
  };

  template <typename A, typename B, auto f>
  struct SecondTestFunctor {
    using FuncType = decltype(f);

    static std::string getPrettyName() {
      return prettyFunctionForValue<FuncType, f>();
    }
    std::pair<std::string, std::string> operator()() {
      return {getPrettyName(), getExpectedTypeName<FuncType, f>()};
    }
  };
};

using TE = vt::util::demangle::TemplateExtract;

TEST_F(TestDemanglerUtils, test_getVoidFuncStrArgs) {
  std::array<std::pair<std::string, std::string>, 10> const data = {{
    {"", ""},
    {"not-starting-void", "not-starting-void"},
    {"void(with-extra-at)end", "void(with-extra-at)end"},
    {"void ()", ""},
    {"void (foo)", "foo"},
    {"void(foo)", "foo"},
    {"void (dontcare-unbalanced))", "dontcare-unbalanced)"},
    {"void (fn())", "fn()"},
    {"void (fn(a))", "fn(a)"},
    {"void (fn(*)(a,b,c,d))", "fn(*)(a,b,c,d)"}
  }};

  for (auto const& [given, expected] : data) {
    EXPECT_EQ(TE::getVoidFuncStrArgs(given), expected);
  }
}

TEST_F(TestDemanglerUtils, test_getNamespace) {
  std::array<std::pair<std::string, std::string>, 6> const data = {{
    {"", ""},
    {"aa", ""},
    {"aa::bb", "aa"},
    {"&aa::bb", "aa"},
    {"aa::dd<bb::cc<r>>::bare", "aa::dd<bb::cc<r>>"},
    {"aa<a>::dd<d>::bare<e>", "aa<a>::dd<d>"}
  }};

  for (auto const& [given, expected] : data) {
    EXPECT_EQ(TE::getNamespace(given), expected);
  }
}

TEST_F(TestDemanglerUtils, test_getBarename) {
  std::array<std::pair<std::string, std::string>, 5> const data = {{
    {"", ""},
    {"aa", "aa"},
    {"aa::bb", "bb"},
    {"aa::dd<bb::cc<r>>::bare", "bare"},
    {"aa<a>::dd<d>::bare<e>", "bare<e>"}
  }};

  for (auto const& [given, expected] : data) {
    EXPECT_EQ(TE::getBarename(given), expected);
  }
}

namespace helpers {
void someFunc_0() { }

int someFunc_1(int a) {
  return a * 2;
}

template <typename A, typename B>
void someFunc_2(A, B) { }
} // namespace helpers

TEST_F(TestDemanglerUtils, test_lastNamedPfType) {
  std::array<std::pair<std::string, std::string>, 8> const data = {{
    {"", ""},
    {std::string{__PRETTY_FUNCTION__}, std::string{}},
    TestFunctor<helpers::someFunc_0>{}(),
    TestFunctor<helpers::someFunc_1>{}(),
    TestFunctor<helpers::someFunc_2<int, float>>{}(),
    SecondTestFunctor<int, float, helpers::someFunc_0>{}(),
    SecondTestFunctor<double, char, helpers::someFunc_1>{}(),
    SecondTestFunctor<unsigned, int, helpers::someFunc_2<int, float>>{}()
  }};

  for (auto const& [spf, expected] : data) {
    EXPECT_EQ(TE::lastNamedPfType(spf, "PF_VALUE_NAME"), expected)
      << "spf: " << spf << std::endl;
  }
}

}}} /* end namespace vt::tests::unit */
