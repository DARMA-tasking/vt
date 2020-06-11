/*
//@HEADER
// *****************************************************************************
//
//                      test_collection_construct_common.h
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

#if !defined INCLUDED_COLLECTION_TEST_COLLECTION_CONSTRUCT_COMMON_H
#define INCLUDED_COLLECTION_TEST_COLLECTION_CONSTRUCT_COMMON_H

#include "test_parallel_harness.h"
#include "test_collection_common.h"
#include "data_message.h"

#include "vt/transport.h"

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <gtest/gtest.h>

#include <cstdint>
#include <tuple>
#include <string>
#include <memory>

#define PRINT_CONSTRUCTOR_VALUES 0

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

struct ConstructHandlers {
  template <
    typename CollectionT,
    typename MessageT = typename CollectionT::MsgType
  >
  static void handler(MessageT* msg, CollectionT* col) {
    // fmt::print(
    //   "{}: constructed TestCol: idx.x()={}\n",
    //   theContext()->getNode(), col->getInex().x(), print_ptr(col)
    // );
    EXPECT_TRUE(col->isConstructed());
  }
};

template <typename CollectionT>
struct TestConstruct : TestParallelHarness {};

template <typename CollectionT>
struct TestConstructDist : TestParallelHarness {};

template <typename ColT, typename Tuple>
struct ConstructParams {
  using IndexType = typename ColT::IndexType;
  using ProxyType = CollectionIndexProxy<ColT,IndexType>;
  static ProxyType constructTup(IndexType idx,Tuple args) {
    return construct(idx,args);
  }
  template <typename... Args>
  static ProxyType construct(IndexType idx, std::tuple<Args...> args) {
    return unpack(idx,args,std::index_sequence_for<Args...>{});
  }
  template <std::size_t ...I>
  static ProxyType unpack(
    IndexType idx, Tuple args, std::index_sequence<I...>
  ) {
    return theCollection()->construct<ColT>(idx,std::get<I>(args)...);
  }

  static ProxyType constructTupCollective(IndexType idx,Tuple args) {
    return constructCollective(idx,args);
  }
  template <typename... Args>
  static ProxyType constructCollective(
    IndexType idx, std::tuple<Args...> args
  ) {
    return unpackCollective(idx,args,std::index_sequence_for<Args...>{});
  }
  template <std::size_t ...I>
  static ProxyType unpackCollective(
    IndexType idx, Tuple args, std::index_sequence<I...>
  ) {
    return theCollection()->constructCollective<ColT>(
      idx,[=](IndexType my_idx) {
        return std::make_unique<ColT>(std::get<I>(args)...);
      }
    );
  }
};

struct BaseCol {
  BaseCol() = default;
  explicit BaseCol(bool const in_constructed)
    : constructed_(in_constructed)
  {}
  bool isConstructed() const { return constructed_; }
protected:
  bool constructed_ = false;
};

TYPED_TEST_SUITE_P(TestConstruct);
TYPED_TEST_SUITE_P(TestConstructDist);

TYPED_TEST_P(TestConstruct, test_construct_1) {
  using ColType   = TypeParam;
  using MsgType   = typename ColType::MsgType;
  using TestParamType = typename ColType::ParamType;

  auto const& this_node = theContext()->getNode();
  if (this_node == 0) {
    auto const& col_size = 32;
    auto rng = TestIndex(col_size);
    TestParamType args = ConstructTuple<TestParamType>::construct();
    auto proxy = ConstructParams<ColType,TestParamType>::constructTup(rng,args);
    auto msg = makeMessage<MsgType>();
    proxy.template broadcast<
      MsgType,
      ConstructHandlers::handler<ColType,MsgType>
    >(msg.get());
  }
}

TYPED_TEST_P(TestConstructDist, test_construct_distributed_1) {
  using ColType   = TypeParam;
  using MsgType   = typename ColType::MsgType;
  using TestParamType = typename ColType::ParamType;

  auto const& col_size = 32;
  auto rng = TestIndex(col_size);
  TestParamType args = ConstructTuple<TestParamType>::construct();
  auto proxy = ConstructParams<ColType,TestParamType>::constructTupCollective(
    rng,args
  );
  auto msg = makeMessage<MsgType>();
  proxy.template broadcast<
    MsgType,
    ConstructHandlers::handler<ColType,MsgType>
  >(msg.get());
}

REGISTER_TYPED_TEST_SUITE_P(TestConstruct,     test_construct_1);
REGISTER_TYPED_TEST_SUITE_P(TestConstructDist, test_construct_distributed_1);

}}} // end namespace vt::tests::unit

#endif /*INCLUDED_COLLECTION_TEST_COLLECTION_CONSTRUCT_COMMON_H*/
