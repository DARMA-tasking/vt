/*
//@HEADER
// *****************************************************************************
//
//                               test_mapping.cc
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

#include <vt/transport.h>

#include <gtest/gtest.h>

#include "test_parallel_harness.h"

namespace vt { namespace tests { namespace unit { namespace mapping {

using namespace vt;
using namespace vt::tests::unit;

template <typename IndexT>
struct MappingTest : vt::Collection<MappingTest<IndexT>, IndexT> {
  MappingTest() {
    fmt::print(
      "{}: constructing on node {}\n", this->getIndex(), theContext()->getNode()
    );
  }
};

template <typename IndexT>
struct MyMapper : vt::mapping::BaseMapper<IndexT> {
  static vt::ObjGroupProxyType construct() {
    return vt::theObjGroup()->makeCollective<MyMapper<IndexT>>().getProxy();
  }

  vt::NodeType map(IndexT* idx, int ndim, vt::NodeType num_nodes) override {
    uint64_t val = 0;
    for (int i = 0; i < ndim; i++) {
      auto dval = static_cast<uint64_t>(idx->get(i));
      val ^= dval << (i * 16);
    }
    return val % num_nodes;
  }
};

template <typename IndexT>
struct WorkMsg : CollectionMessage<MappingTest<IndexT>> {};

static int32_t num_work = 0;

template <typename IndexT>
void work(WorkMsg<IndexT>* msg, MappingTest<IndexT>* elm) {
  fmt::print(
    "node={}: num_work={}, idx={}\n", theContext()->getNode(), num_work,
    elm->getIndex()
  );
  num_work++;
}

static constexpr int32_t const num_elms_per_node = 8;

template <typename IndexT>
struct TestMapping : TestParallelHarness { };

template <typename T>
std::vector<T> kFactors(T n, int8_t k) {
  T const orig_n = n;
  std::vector<T> factors;
  while (n % 2 == 0) {
    factors.push_back(2);
    n /= 2;
  }
  for (T i = 3; i*i <= n; i += 2) {
    while (n % i == 0) {
      factors.push_back(i);
      n /= i;
    }
  }
  if (n > 2) {
    factors.push_back(n);
  }
  while (factors.size() < static_cast<std::size_t>(k)) {
    factors.push_back(1);
  }

  std::vector<T> output = factors;
  // Now, if need be, compress them down to k
  if (factors.size() > static_cast<std::size_t>(k)) {
    std::vector<T> compressed;
    std::size_t cur = 0;
    for (int i = 0; i < k; i++) {
      compressed.emplace_back(1);
      auto len = i == k-1 ? factors.size() - cur : factors.size() / k;
      for (std::size_t j = 0; j < len; j++) {
        if (cur < factors.size()) {
          compressed[i] *= factors[cur++];
        }
      }
    }
    output = compressed;
  }

  T total = 1;
  std::string buf = "";
  for (auto&& elm : output) {
    buf += fmt::format("{}, ", elm);
    total *= elm;
  }
  buf += "\n";
  vt_debug_print(
    terse, gen,
    "n={}, k={}, factors={}; total={}\n",
    orig_n, k, buf, total
  );
  vtAssert(total == orig_n, "Must be equal if this worked");
  return output;
}

TYPED_TEST_SUITE_P(TestMapping);

TYPED_TEST_P(TestMapping, test_custom_mapping_1) {
  using IndexType  = typename std::tuple_element<0,TypeParam>::type;
  using MapperType = typename std::tuple_element<1,TypeParam>::type;
  using ColType    = MappingTest<IndexType>;

  auto const this_node = theContext()->getNode();
  auto const num_nodes = theContext()->getNumNodes();

  num_work = 0;

  auto const total = num_nodes * num_elms_per_node;
  IndexType range = {};
  auto ndims = range.ndims();

  auto factors = kFactors(total, ndims);
  vtAssertExpr(factors.size() == static_cast<std::size_t>(ndims));
  for (index::NumDimensionsType i = 0; i < ndims; i++) {
    range[i] = factors[i];
  }

  fmt::print("total range is {}\n", range);

  auto my_proxy_raw = MapperType::construct();
  objgroup::proxy::Proxy<MapperType> my_proxy{my_proxy_raw};

  int counter = 0;
  range.foreach([&](IndexType test_idx) {
    if (my_proxy.get()->map(&test_idx, ndims, num_nodes) == this_node) {
      counter++;
    }
  });

  auto proxy = vt::makeCollection<ColType>()
    .bulkInsert(range)
    .mapperObjGroup(my_proxy)
    .wait();

  vt::runInEpochCollective([&]{
    proxy.template broadcastCollective<WorkMsg<IndexType>, work<IndexType>>();
  });

  EXPECT_EQ(counter, num_work);
  num_work = 0;

  auto proxy2 = vt::makeCollection<ColType>()
    .bulkInsert(range)
    .template mapperObjGroupConstruct<MapperType>()
    .wait();

  vt::runInEpochCollective([&]{
    proxy2.template broadcastCollective<WorkMsg<IndexType>, work<IndexType>>();
  });

  EXPECT_EQ(counter, num_work);
}

///////////////////////////////////////////////////////////////////////////////
// Implement a distributed mapper that communicates to determine the mapping of
// each element
///////////////////////////////////////////////////////////////////////////////

template <typename IndexT>
struct MyDistMapper : vt::mapping::BaseMapper<IndexT> {
  static vt::ObjGroupProxyType construct() {
    auto proxy =  vt::theObjGroup()->makeCollective<MyDistMapper<IndexT>>();
    proxy.get()->proxy = proxy;
    return proxy.getProxy();
  }

  MyDistMapper()
    : my_state((theContext()->getNode() * 2993ull) << 5)
  { }

  struct GetMapMsg : vt::Message {
    GetMapMsg() = default;
    GetMapMsg(IndexT in_idx, NodeType in_request_node)
      : idx_(in_idx),
        request_node_(in_request_node)
    { }
    IndexT idx_ = {};
    NodeType request_node_ = uninitialized_destination;
  };

  struct AnswerMsg : vt::Message {
    AnswerMsg() = default;
    explicit AnswerMsg(NodeType in_answer)
      : answer_(in_answer)
    { }
    IndexT idx_ = {};
    NodeType answer_ = uninitialized_destination;
  };

  vt::NodeType map(IndexT* idx, int ndim, vt::NodeType num_nodes) override {
    uint64_t val = 0;
    for (int i = 0; i < ndim; i++) {
      auto dval = static_cast<uint64_t>(idx->get(i));
      val ^= dval << (i * 16);
    }
    auto const owner = static_cast<NodeType>(val % num_nodes);
    //vt_print(gen, "map: idx={}, ndim={}, owner={}\n", *idx, ndim, owner);
    if (owner == theContext()->getNode()) {
      /// get to decide the mapping
      return (val ^ my_state) % num_nodes;
    } else {
      // runInEpochRooted is not DS?
      auto ep = theTerm()->makeEpochRooted("mapTest", term::UseDS{true});
      theMsg()->pushEpoch(ep);
      proxy[owner].template send<GetMapMsg, &MyDistMapper<IndexT>::getMap>(
        *idx, theContext()->getNode()
      );
      theMsg()->popEpoch(ep);
      theTerm()->finishedEpoch(ep);
      vt::runSchedulerThrough(ep);
      vtAssertExpr(cur_answer != uninitialized_destination);
      auto const ret = cur_answer;
      cur_answer = uninitialized_destination;
      return ret;
    }
  }

  void getMap(GetMapMsg* msg) {
    //vt_print(gen, "getMap: idx={}, request_node={}\n", msg->idx_, msg->request_node_);
    auto node = map(&msg->idx_, msg->idx_.ndims(), theContext()->getNumNodes());
    auto r = msg->request_node_;
    proxy[r].template send<AnswerMsg, &MyDistMapper<IndexT>::answer>(node);
  }

  void answer(AnswerMsg* msg) {
    //vt_print(gen, "answer: answer={}\n", msg->answer_);
    vtAssertExpr(cur_answer == uninitialized_destination);
    cur_answer = msg->answer_;
  }

private:
  uint64_t my_state = 0;
  NodeType cur_answer = uninitialized_destination;
  objgroup::proxy::Proxy<MyDistMapper<IndexT>> proxy;
};

using IndexTestTypes = testing::Types<
  std::tuple<vt::Index1D, MyMapper<vt::Index1D>>,
  std::tuple<vt::Index2D, MyMapper<vt::Index2D>>,
  std::tuple<vt::Index3D, MyMapper<vt::Index3D>>,
  std::tuple<vt::Index1D, MyDistMapper<vt::Index1D>>,
  std::tuple<vt::Index2D, MyDistMapper<vt::Index2D>>,
  std::tuple<vt::Index3D, MyDistMapper<vt::Index3D>>
>;

REGISTER_TYPED_TEST_SUITE_P(TestMapping, test_custom_mapping_1);

INSTANTIATE_TYPED_TEST_SUITE_P(
  test_all_mappings, TestMapping, IndexTestTypes, DEFAULT_NAME_GEN
);

}}}} // end namespace vt::tests::unit::mapping
