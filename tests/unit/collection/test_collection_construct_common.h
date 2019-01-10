/*
//@HEADER
// ************************************************************************
//
//                          test_collection_construct_common.h
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

#if !defined INCLUDED_COLLECTION_TEST_COLLECTION_CONSTRUCT_COMMON_H
#define INCLUDED_COLLECTION_TEST_COLLECTION_CONSTRUCT_COMMON_H

#include "test_parallel_harness.h"
#include "test_collection_common.h"
#include "data_message.h"

#include "vt/transport.h"

#include <fmt/format.h>
#include <fmt/ostream.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

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
      idx,[=](IndexType idx) {
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

TYPED_TEST_CASE_P(TestConstruct);
TYPED_TEST_CASE_P(TestConstructDist);

TYPED_TEST_P(TestConstruct, test_construct_1) {
  using ColType   = TypeParam;
  using MsgType   = typename ColType::MsgType;
  using ParamType = typename ColType::ParamType;

  auto const& this_node = theContext()->getNode();
  if (this_node == 0) {
    auto const& col_size = 32;
    auto rng = TestIndex(col_size);
    ParamType args = ConstructTuple<ParamType>::construct();
    auto proxy = ConstructParams<ColType,ParamType>::constructTup(rng,args);
    auto msg = makeSharedMessage<MsgType>();
    proxy.template broadcast<
      MsgType,
      ConstructHandlers::handler<ColType,MsgType>
    >(msg);
  }
}

TYPED_TEST_P(TestConstructDist, test_construct_distributed_1) {
  using ColType   = TypeParam;
  using MsgType   = typename ColType::MsgType;
  using ParamType = typename ColType::ParamType;

  auto const& col_size = 32;
  auto rng = TestIndex(col_size);
  ParamType args = ConstructTuple<ParamType>::construct();
  auto proxy = ConstructParams<ColType,ParamType>::constructTupCollective(
    rng,args
  );
  auto msg = makeSharedMessage<MsgType>();
  proxy.template broadcast<
    MsgType,
    ConstructHandlers::handler<ColType,MsgType>
  >(msg);
}

REGISTER_TYPED_TEST_CASE_P(TestConstruct,     test_construct_1);
REGISTER_TYPED_TEST_CASE_P(TestConstructDist, test_construct_distributed_1);

}}} // end namespace vt::tests::unit

#endif /*INCLUDED_COLLECTION_TEST_COLLECTION_CONSTRUCT_COMMON_H*/
