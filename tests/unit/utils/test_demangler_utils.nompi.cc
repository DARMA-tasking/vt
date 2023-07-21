/*
//@HEADER
// *****************************************************************************
//
//                        test_demangler_utils.nompi.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#include "vt/utils/demangle/demangle.h"
#include "test_harness.h"

namespace vt { namespace tests { namespace unit {

struct TestDemanglerUtils : TestHarness {
  template <class MyType, MyType PF_VALUE_NAME>
  static std::string getExpectedTypeName() {
    std::string signature = __PRETTY_FUNCTION__;
    auto typePos = signature.rfind("PF_VALUE_NAME");
    auto lastBracket = signature.rfind("]");

    assert(typePos != std::string::npos);
    assert(lastBracket != std::string::npos);

    return signature.substr(typePos + 16, lastBracket - (typePos + 16));
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

  template <typename A, typename B, typename F, F f>
  struct SecondTestFunctor {
    static std::string getPrettyName() {
      return prettyFunctionForValue<F, f>();
    }
    std::pair<std::string, std::string> operator()() {
      return {getPrettyName(), getExpectedTypeName<F, f>()};
    }
  };
};

using TE = vt::util::demangle::TemplateExtract;

TEST_F(TestDemanglerUtils, test_getVoidFuncStrArgs) {
  std::vector<std::tuple<std::string, std::string>> data;
  data.emplace_back("", "");
  data.emplace_back("not-starting-void", "not-starting-void");
  data.emplace_back("void(with-extra-at)end", "void(with-extra-at)end");
  data.emplace_back("void ()", "");
  data.emplace_back("void (foo)", "foo");
  data.emplace_back("void(foo)", "foo");
  data.emplace_back("void (dontcare-unbalanced))", "dontcare-unbalanced)");
  data.emplace_back("void (fn())", "fn()");
  data.emplace_back("void (fn(a))", "fn(a)");
  data.emplace_back("void (fn(*)(a,b,c,d))", "fn(*)(a,b,c,d)");

  for (auto& t : data) {
    std::string& given = std::get<0>(t);
    std::string& expected = std::get<1>(t);
    EXPECT_EQ(TE::getVoidFuncStrArgs(given), expected);
  }
}

TEST_F(TestDemanglerUtils, test_getNamespace) {
  std::vector<std::tuple<std::string, std::string>> data;
  data.emplace_back("", "");
  data.emplace_back("aa", "");
  data.emplace_back("aa::bb", "aa");
  data.emplace_back("&aa::bb", "aa");
  data.emplace_back("aa::dd<bb::cc<r>>::bare", "aa::dd<bb::cc<r>>");
  data.emplace_back("aa<a>::dd<d>::bare<e>", "aa<a>::dd<d>");

  for (auto& t : data) {
    std::string& given = std::get<0>(t);
    std::string& expected = std::get<1>(t);
    EXPECT_EQ(TE::getNamespace(given), expected);
  }
}

TEST_F(TestDemanglerUtils, test_getBarename) {
  std::vector<std::tuple<std::string, std::string>> data;
  data.emplace_back("", "");
  data.emplace_back("aa", "aa");
  data.emplace_back("aa::bb", "bb");
  data.emplace_back("aa::dd<bb::cc<r>>::bare", "bare");
  data.emplace_back("aa<a>::dd<d>::bare<e>", "bare<e>");

  for (auto& t : data) {
    std::string& given = std::get<0>(t);
    std::string& expected = std::get<1>(t);
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
  std::vector<std::tuple<std::string, std::string>> data;
  data.emplace_back("", "");
  data.emplace_back(__PRETTY_FUNCTION__, "");
  data.emplace_back(TestFunctor<helpers::someFunc_0>{}());
  data.emplace_back(TestFunctor<helpers::someFunc_1>{}());
  data.emplace_back(TestFunctor<helpers::someFunc_2<int, float>>{}());
  data.emplace_back(
    SecondTestFunctor<
      int, float, decltype(helpers::someFunc_0), helpers::someFunc_0>{}());
  data.emplace_back(
    SecondTestFunctor<
      double, char, decltype(helpers::someFunc_1), helpers::someFunc_1>{}());
  data.emplace_back(SecondTestFunctor<
                    unsigned, int, decltype(helpers::someFunc_2<int, float>),
                    helpers::someFunc_2<int, float>>{}());

  for (auto& t : data) {
    std::string& spf = std::get<0>(t);
    std::string& expected = std::get<1>(t);
    EXPECT_EQ(TE::lastNamedPfType(spf, "PF_VALUE_NAME"), expected)
      << "spf: " << spf << std::endl;
  }
}

}}} /* end namespace vt::tests::unit */
