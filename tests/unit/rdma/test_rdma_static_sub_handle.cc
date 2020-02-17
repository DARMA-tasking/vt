/*
//@HEADER
// *****************************************************************************
//
//                       test_rdma_static_sub_handle.cc
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

#include <gtest/gtest.h>

#include "vt/transport.h"
#include "test_parallel_harness.h"

namespace vt { namespace tests { namespace unit {

struct TestObjGroup {
  using ProxyType = vt::objgroup::proxy::Proxy<TestObjGroup>;

  TestObjGroup() = default;

  void initialize(ProxyType in_proxy) {
    proxy_ = in_proxy;
  }

  template <typename T>
  vt::rdma::HandleSet<T> makeHandleSet(
    int32_t max_elm, std::unordered_map<int32_t, std::size_t> map, bool uniform
  ) {
    return proxy_.template makeHandleSetRDMA<T>(max_elm, map, uniform);
  }

  static ProxyType construct() {
    auto proxy = vt::theObjGroup()->makeCollective<TestObjGroup>();
    proxy.get()->initialize(proxy);
    return proxy;
  }

private:
  ProxyType proxy_;
};

template <typename T>
struct UpdateData {
  template <typename HandleT>
  static void init(
    HandleT& handle, int space, std::size_t size, vt::NodeType rank
) {
    auto idx = handle.getIndex();
    handle.modifyExclusive(idx, [=](T* val){
      setMem(val, space, size, rank, 0);
    });
  }

  static void setMem(
    T* ptr, int space, std::size_t size, vt::NodeType rank, std::size_t offset
  ) {
    for (std::size_t i = offset; i < size; i++) {
      ptr[i] = static_cast<T>(space * rank + i);
    }
  }

  static void test(
    std::unique_ptr<T[]> ptr, int space, std::size_t size, vt::NodeType rank,
    std::size_t offset, T val = T{}
  ) {
    for (std::size_t i = offset; i < size; i++) {
      EXPECT_EQ(ptr[i], static_cast<T>(space * rank + i + val));
    }
  }
};

template <typename T>
struct TestRDMAHandleSet : TestParallelHarness { };

TYPED_TEST_CASE_P(TestRDMAHandleSet);

TYPED_TEST_P(TestRDMAHandleSet, test_rdma_handle_set_1) {
  using T = TypeParam;
  auto proxy = TestObjGroup::construct();

  int32_t num_hans = 4;
  std::size_t num_vals = 8;
  int space = 100;
  std::unordered_map<int32_t, std::size_t> map;
  for (int i = 0; i < num_hans; i++) {
    map[i] = num_vals;
  }
  auto han_set = proxy.get()->makeHandleSet<T>(num_hans, map, true);

  auto this_node = theContext()->getNode();
  for (int i = 0; i < num_hans; i++) {
    auto idx_rank = this_node * num_hans + i;
    UpdateData<T>::init(han_set[i], space, num_vals, idx_rank);
  }

  // Barrier to order following locks
  vt::theCollective()->barrier();

  auto num_nodes = theContext()->getNumNodes();
  for (int node = 0; node < num_nodes; node++) {
    for (int han = 0; han < num_hans; han++) {
      auto idx_rank = node * num_hans + han;
      vt::Index2D idx(node, han);
      auto ptr = std::make_unique<T[]>(num_vals);
      han_set->get(idx, &ptr[0], num_vals, 0, vt::Lock::Shared);
      UpdateData<T>::test(std::move(ptr), space, num_vals, idx_rank, 0);
    }
  }

  // Barrier to order following locks
  vt::theCollective()->barrier();

  proxy.destroyHandleSetRDMA(han_set);
}

TYPED_TEST_P(TestRDMAHandleSet, test_rdma_handle_set_2) {
  using T = TypeParam;
  auto proxy = TestObjGroup::construct();

  auto this_node = theContext()->getNode();
  auto num_nodes = theContext()->getNumNodes();
  auto max_hans = num_nodes * num_nodes;
  int32_t num_hans = num_nodes * (this_node + 1);
  int32_t offset = this_node;
  int space = 100;
  std::unordered_map<int32_t, std::size_t> map;
  for (int i = offset; i < num_hans + offset; i++) {
    map[i] = (this_node + 1) + i;
  }
  auto han_set = proxy.get()->makeHandleSet<T>(max_hans + num_nodes, map, false);

  for (int i = offset; i < num_hans + offset; i++) {
    auto idx_rank = this_node * max_hans + i;
    UpdateData<T>::init(han_set[i], space, (this_node + 1) + i, idx_rank);
  }

  // Barrier to order following locks
  vt::theCollective()->barrier();

  for (int node = 0; node < num_nodes; node++) {
    for (int han = node; han < (num_nodes * (node + 1)) + node; han++) {
      vt::Index2D idx(node, han);
      auto size = han_set->getSize(idx);
      EXPECT_EQ(size, static_cast<std::size_t>((node + 1) + han));

      auto idx_rank = node * max_hans + han;
      auto ptr = std::make_unique<T[]>(size);
      han_set->get(idx, &ptr[0], size, 0, vt::Lock::Shared);
      UpdateData<T>::test(std::move(ptr), space, size, idx_rank, 0);
    }
  }

  // Barrier to order following locks
  vt::theCollective()->barrier();

  proxy.destroyHandleSetRDMA(han_set);
}

using RDMASetTestTypes = testing::Types<
  int,
  double,
  float,
  int32_t,
  int64_t,
  uint64_t,
  int64_t,
  int16_t,
  uint16_t
>;

REGISTER_TYPED_TEST_CASE_P(
  TestRDMAHandleSet,
  test_rdma_handle_set_1,
  test_rdma_handle_set_2
);

INSTANTIATE_TYPED_TEST_CASE_P(
  test_rdma_handle_set, TestRDMAHandleSet, RDMASetTestTypes
);

}}} /* end namespace vt::tests::unit */
