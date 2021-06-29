/*
//@HEADER
// *****************************************************************************
//
//                   test_rdma_static_sub_handle.extended.cc
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

#include "vt/objgroup/manager.h"
#include "test_parallel_harness.h"
#include "test_rdma_common.h"

namespace vt { namespace tests { namespace unit {

struct TestObjGroupExt {
  using ProxyType = vt::objgroup::proxy::Proxy<TestObjGroupExt>;

  TestObjGroupExt() = default;

  void initialize(ProxyType in_proxy) {
    proxy_ = in_proxy;
  }

  template <typename T>
  vt::rdma::HandleSet<T> makeHandleSet(
    int32_t max_elm, std::unordered_map<int32_t, std::size_t> map,
    bool dense_start_at_zero, bool uniform
  ) {
    if (dense_start_at_zero) {
      std::vector<std::size_t> vec;
      for (int i = 0; i < static_cast<int>(map.size()); i++) {
        auto iter = map.find(i);
        vtAssertExpr(iter != map.end());
        vec.push_back(iter->second);
      }
      return proxy_.template makeHandleSetRDMA<T>(max_elm, vec, uniform);
    } else {
      return proxy_.template makeHandleSetRDMA<T>(max_elm, map, uniform);
    }
  }

  static ProxyType construct() {
    auto proxy = vt::theObjGroup()->makeCollective<TestObjGroupExt>();
    proxy.get()->initialize(proxy);
    return proxy;
  }

private:
  ProxyType proxy_;
};

template <typename T>
struct TestRDMAHandleSet : TestParallelHarness { };

TYPED_TEST_SUITE_P(TestRDMAHandleSet);

TYPED_TEST_P(TestRDMAHandleSet, test_rdma_handle_set_1) {
  using T = TypeParam;
  auto proxy = TestObjGroupExt::construct();

  int32_t num_hans = 4;
  std::size_t num_vals = 8;
  int space = 100;
  std::unordered_map<int32_t, std::size_t> map;
  for (int i = 0; i < num_hans; i++) {
    map[i] = num_vals;
  }
  auto han_set = proxy.get()->makeHandleSet<T>(num_hans, map, true, true);

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

  // Barrier to finish all pending operations before destroy starts
  vt::theCollective()->barrier();

  proxy.destroyHandleSetRDMA(han_set);
}

TYPED_TEST_P(TestRDMAHandleSet, test_rdma_handle_set_2) {
  using T = TypeParam;
  auto proxy = TestObjGroupExt::construct();

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
  auto han_set = proxy.get()->makeHandleSet<T>(
    max_hans + num_nodes, map, false, false
  );

  for (int i = offset; i < num_hans + offset; i++) {
    auto idx_rank = this_node * max_hans + i;
    UpdateData<T>::init(han_set[i], space, (this_node + 1) + i, idx_rank);
  }

  // Barrier to order following locks
  vt::theCollective()->barrier();

  for (int node = 0; node < num_nodes; node++) {
    for (int han = node; han < (num_nodes * (node + 1)) + node; han++) {
      vt::Index2D idx(node, han);
      auto count = han_set->getCount(idx);
      EXPECT_EQ(count, static_cast<std::size_t>((node + 1) + han));

      auto idx_rank = node * max_hans + han;
      auto ptr = std::make_unique<T[]>(count);
      han_set->get(idx, &ptr[0], count, 0, vt::Lock::Shared);
      UpdateData<T>::test(std::move(ptr), space, count, idx_rank, 0);
    }
  }

  // Barrier to finish all pending operations before destroy starts
  vt::theCollective()->barrier();

  proxy.destroyHandleSetRDMA(han_set);
}

TYPED_TEST_P(TestRDMAHandleSet, test_rdma_handle_set_3) {
  using T = TypeParam;
  auto proxy = TestObjGroupExt::construct();

  auto this_node = theContext()->getNode();
  auto num_nodes = theContext()->getNumNodes();
  int32_t num_hans = 4;
  int space = 100;
  std::unordered_map<int32_t, std::size_t> map;
  for (int i = 0; i < num_hans; i++) {
    map[i] = (this_node + 1) * (i + 1);
  }
  auto han_set = proxy.get()->makeHandleSet<T>(num_hans, map, true, false);

  for (int i = 0; i < num_hans; i++) {
    auto idx_rank = this_node * num_hans + i;
    UpdateData<T>::init(han_set[i], space, (this_node + 1) * (i + 1), idx_rank);
  }

  // Barrier to order following locks
  vt::theCollective()->barrier();

  for (int node = 0; node < num_nodes; node++) {
    for (int han = 0; han < num_hans; han++) {
      vt::Index2D idx(node, han);
      auto count = han_set->getCount(idx);
      EXPECT_EQ(count, static_cast<std::size_t>((node + 1) * (han + 1)));
    }
  }

  // Barrier to order following locks
  vt::theCollective()->barrier();

  std::vector<std::unique_ptr<T[]>> accums;
  std::vector<rdma::RequestHolder> holders;

  for (int node = 0; node < num_nodes; node++) {
    for (int han = 0; han < num_hans; han++) {
      vt::Index2D idx(node, han);
      std::size_t size = (node + 1) * (han + 1);

      accums.emplace_back(std::make_unique<T[]>(size));
      int cur = accums.size()-1;
      for (std::size_t i = 0; i < size; i++) {
        accums[cur][i] = static_cast<T>(1);
      }
      holders.emplace_back(
        han_set->raccum(idx, &accums[cur][0], size, 0, MPI_SUM, vt::Lock::Shared)
      );
    }
  }

  // Wait for all accums to finish
  for (auto&& elm : holders) {
    elm.wait();
  }

  // Clear the memory holders for the accums
  accums.clear();

  // Wait for all global accums to complete
  vt::theCollective()->barrier();

  for (int node = 0; node < num_nodes; node++) {
    for (int han = 0; han < num_hans; han++) {
      vt::Index2D idx(node, han);
      std::size_t size = (node + 1) * (han + 1);

      auto idx_rank = node * num_hans + han;
      auto ptr = std::make_unique<T[]>(size);
      auto req = han_set->rget(idx, &ptr[0], size, 0, vt::Lock::Exclusive);
      req.wait();
      UpdateData<T>::test(std::move(ptr), space, size, idx_rank, 0, num_nodes);
    }
  }

  // Barrier to finish all pending operations before destroy starts
  vt::theCollective()->barrier();


  proxy.destroyHandleSetRDMA(han_set);
}

TYPED_TEST_P(TestRDMAHandleSet, test_rdma_handle_set_4) {
  using T = TypeParam;
  auto proxy = TestObjGroupExt::construct();

  auto this_node = theContext()->getNode();
  auto num_nodes = theContext()->getNumNodes();
  auto next = this_node + 1 < num_nodes ? this_node + 1 : 0;
  int32_t num_hans = 4;
  std::size_t num_vals = 10;
  int space = 100;
  std::unordered_map<int32_t, std::size_t> map;
  for (int i = 0; i < num_hans; i++) {
    map[i] = num_vals;
  }
  auto han_set = proxy.get()->makeHandleSet<T>(num_hans, map, true, true);

  for (int i = 0; i < num_hans; i++) {
    auto idx_rank = this_node * num_hans + i;
    UpdateData<T>::init(han_set[i], space, num_vals, idx_rank);
  }

  // Barrier to order following locks
  vt::theCollective()->barrier();

  for (int han = 0; han < num_hans; han++) {
    vt::Index2D idx(next, han);
    auto ptr = std::make_unique<T[]>(num_vals/2);
    auto idx_rank = this_node * num_hans + han;
    UpdateData<T>::setMem(&ptr[0], space, num_vals/2, idx_rank, num_vals/2);
    han_set->put(idx, &ptr[0], num_vals/2, num_vals/2, vt::Lock::Exclusive);
  }

  // Barrier to order following locks
  vt::theCollective()->barrier();

  for (int han = 0; han < num_hans; han++) {
    vt::Index2D idx(next, han);
    auto ptr = std::make_unique<T[]>(num_vals);
    auto ptr2 = std::make_unique<T[]>(num_vals);
    han_set->get(idx, &ptr[0], num_vals, 0, vt::Lock::Shared);
    for (std::size_t i  = 0; i < num_vals; i++) {
      ptr2[i] = ptr[i];
    }
    auto idx_rank = this_node * num_hans + han;
    auto idx_next_rank = next * num_hans + han;
    UpdateData<T>::test(std::move(ptr), space, num_vals/2, idx_next_rank, 0);
    UpdateData<T>::test(std::move(ptr2), space, num_vals/2, idx_rank, num_vals/2);
  }

  // Barrier to finish all pending operations before destroy starts
  vt::theCollective()->barrier();

  proxy.destroyHandleSetRDMA(han_set);
}

TYPED_TEST_P(TestRDMAHandleSet, test_rdma_handle_set_5) {
  using T = TypeParam;
  auto proxy = TestObjGroupExt::construct();

  auto this_node = theContext()->getNode();
  auto num_nodes = theContext()->getNumNodes();
  int32_t num_hans = 4;
  std::size_t num_vals = 10;
  int space = 100;
  std::unordered_map<int32_t, std::size_t> map;
  for (int i = 0; i < num_hans; i++) {
    map[i] = num_vals;
  }
  auto han_set = proxy.get()->makeHandleSet<T>(num_hans, map, true, true);

  for (int i = 0; i < num_hans; i++) {
    auto idx_rank = this_node * num_hans + i;
    UpdateData<T>::init(han_set[i], space, num_vals, idx_rank);
  }

  // Barrier to order following locks
  vt::theCollective()->barrier();

  for (int node = 0; node < num_nodes; node++) {
    for (int han = 0; han < num_hans; han++) {
      vt::Index2D idx(node, han);
      for (int i = 0; i < static_cast<int>(num_vals); i++) {
        han_set->fetchOp(idx, 1, i, MPI_SUM, vt::Lock::Shared);
      }
    }
  }

  // Barrier to order following locks
  vt::theCollective()->barrier();

  for (int node = 0; node < num_nodes; node++) {
    for (int han = 0; han < num_hans; han++) {
      vt::Index2D idx(node, han);
      auto idx_rank = node * num_hans + han;
      auto ptr = std::make_unique<T[]>(num_vals);
      han_set->get(idx, &ptr[0], num_vals, 0, vt::Lock::Exclusive);
      UpdateData<T>::test(std::move(ptr), space, num_vals, idx_rank, 0, num_nodes);
    }
  }

  // Barrier to finish all pending operations before destroy starts
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

REGISTER_TYPED_TEST_SUITE_P(
  TestRDMAHandleSet,
  test_rdma_handle_set_1,
  test_rdma_handle_set_2,
  test_rdma_handle_set_3,
  test_rdma_handle_set_4,
  test_rdma_handle_set_5
);

INSTANTIATE_TYPED_TEST_SUITE_P(
  test_rdma_handle_set, TestRDMAHandleSet, RDMASetTestTypes,
  DEFAULT_NAME_GEN
);

}}} /* end namespace vt::tests::unit */
