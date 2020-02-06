/*
//@HEADER
// *****************************************************************************
//
//                            test_rdma_handle.cc
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
  vt::HandleRDMA<T> makeHandle(std::size_t size, bool uniform) {
    return vt::theHandle()->template makeHandleCollectiveObjGroup<
      T, vt::rdma::HandleEnum::StaticSize
    >(proxy_, size, uniform);
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
    handle.modifyExclusive([=](T* val){
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
struct TestRDMAHandle : TestParallelHarness { };

TYPED_TEST_CASE_P(TestRDMAHandle);

TYPED_TEST_P(TestRDMAHandle, test_rdma_handle_1) {
  std::size_t size = 10;

  using T = TypeParam;
  auto proxy = TestObjGroup::construct();
  vt::HandleRDMA<T> handle = proxy.get()->makeHandle<T>(size, true);

  do vt::runScheduler(); while (not handle.ready());

  auto rank = vt::theContext()->getNode();
  int space = 100;
  UpdateData<T>::init(handle, space, size, rank);

  // Barrier to order following locks
  vt::theCollective()->barrier();

  auto num = vt::theContext()->getNumNodes();
  for (vt::NodeType node = 0; node < num; node++) {
    {
      auto ptr = std::make_unique<T[]>(size);
      handle.get(node, &ptr[0], size, 0, vt::Lock::Shared);
      UpdateData<T>::test(std::move(ptr), space, size, node, 0);
    }
    {
      auto ptr = std::make_unique<T[]>(size/2);
      auto req = handle.rget(node, &ptr[0], size/2, size/2, vt::Lock::Shared);
      req.wait();
      UpdateData<T>::test(std::move(ptr), space, size/2, node, size/2);
    }
    {
      auto ptr = std::make_unique<T[]>(1);
      handle.get(node, &ptr[0], 1, size-1, vt::Lock::Shared);
      UpdateData<T>::test(std::move(ptr), space, 1, node, size-1);
    }
  }

  vt::theCollective()->barrier();
  vt::theHandle()->deleteHandleCollectiveObjGroup(handle);
}

TYPED_TEST_P(TestRDMAHandle, test_rdma_handle_2) {
  std::size_t size = 10;

  using T = TypeParam;
  auto proxy = TestObjGroup::construct();
  vt::HandleRDMA<T> handle = proxy.get()->makeHandle<T>(size, true);

  do vt::runScheduler(); while (not handle.ready());

  auto rank = vt::theContext()->getNode();
  int space = 100;
  UpdateData<T>::init(handle, space, size, rank);

  // Barrier to order following locks
  vt::theCollective()->barrier();

  auto num = vt::theContext()->getNumNodes();
  auto next = rank + 1 < num ? rank + 1 : 0;
  //auto prev = rank - 1 >= 0 ? rank - 1 : num-1;
  for (vt::NodeType node = 0; node < num; node++) {
    {
      auto ptr = std::make_unique<T[]>(size);
      handle.get(node, &ptr[0], size, 0, vt::Lock::Shared);
      UpdateData<T>::test(std::move(ptr), space, size, node, 0);
    }
  }

  // Barrier to allow gets to finish
  vt::theCollective()->barrier();

  {
    auto ptr = std::make_unique<T[]>(size/2);
    UpdateData<T>::setMem(&ptr[0], space, size/2, rank, size/2);
    handle.put(next, &ptr[0], size/2, size/2, vt::Lock::Exclusive);
  }

  // Barrier to allow puts to finish
  vt::theCollective()->barrier();

  {
    auto ptr = std::make_unique<T[]>(size);
    auto ptr2 = std::make_unique<T[]>(size);
    handle.get(next, &ptr[0], size, 0, vt::Lock::Shared);
    for (std::size_t i  = 0; i < size; i++) {
      ptr2[i] = ptr[i];
    }
    UpdateData<T>::test(std::move(ptr), space, size/2, next, 0);
    UpdateData<T>::test(std::move(ptr2), space, size/2, rank, size/2);
  }

  vt::theCollective()->barrier();
  vt::theHandle()->deleteHandleCollectiveObjGroup(handle);
}

TYPED_TEST_P(TestRDMAHandle, test_rdma_handle_3) {
  std::size_t size = 10;

  using T = TypeParam;
  auto proxy = TestObjGroup::construct();
  vt::HandleRDMA<T> handle = proxy.get()->makeHandle<T>(size, true);

  do vt::runScheduler(); while (not handle.ready());

  int space = 100;
  UpdateData<T>::init(handle, space, size, 0);

  // Barrier to order following locks
  vt::theCollective()->barrier();

  auto num = vt::theContext()->getNumNodes();
  for (vt::NodeType node = 0; node < num; node++) {
    {
      auto ptr = std::make_unique<T[]>(size);
      for (std::size_t i = 0; i < size; i++) {
        ptr[i] = static_cast<T>(1);
      }
      handle.accum(node, &ptr[0], size, 0, MPI_SUM, vt::Lock::Shared);
    }
  }

  // Barrier to allow gets to finish
  vt::theCollective()->barrier();

  for (vt::NodeType node = 0; node < num; node++) {
    {
      auto ptr = std::make_unique<T[]>(size);
      handle.get(node, &ptr[0], size, 0, vt::Lock::Exclusive);
      UpdateData<T>::test(std::move(ptr), space, size, 0, 0, num);
    }
  }

  vt::theCollective()->barrier();
  vt::theHandle()->deleteHandleCollectiveObjGroup(handle);
}

TYPED_TEST_P(TestRDMAHandle, test_rdma_handle_4) {
  auto rank = vt::theContext()->getNode();
  std::size_t per_size = 10;
  std::size_t size = per_size * (rank + 1);

  using T = TypeParam;
  auto proxy = TestObjGroup::construct();
  vt::HandleRDMA<T> handle = proxy.get()->makeHandle<T>(size, false);

  do vt::runScheduler(); while (not handle.ready());

  // Barrier to order following locks
  vt::theCollective()->barrier();

  auto num = vt::theContext()->getNumNodes();
  for (vt::NodeType node = 0; node < num; node++) {
    EXPECT_EQ(handle.getSize(node), (node + 1) * per_size);
  }

  vt::theCollective()->barrier();
  vt::theHandle()->deleteHandleCollectiveObjGroup(handle);
}

using RDMATestTypes = testing::Types<
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
  TestRDMAHandle,
  test_rdma_handle_1,
  test_rdma_handle_2,
  test_rdma_handle_3,
  test_rdma_handle_4
);

INSTANTIATE_TYPED_TEST_CASE_P(test_rdma_handle, TestRDMAHandle, RDMATestTypes);



}}} /* end namespace vt::tests::unit */
