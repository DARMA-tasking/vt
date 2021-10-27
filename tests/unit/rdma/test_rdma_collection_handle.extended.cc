/*
//@HEADER
// *****************************************************************************
//
//                   test_rdma_collection_handle.extended.cc
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

#include "test_parallel_harness.h"
#include "test_helpers.h"
#include "vt/vrt/collection/manager.h"

namespace vt { namespace tests { namespace unit {

bool triggered_lb = false;

template <typename T>
struct TestCol : vt::Collection<TestCol<T>, vt::Index2D> {
  TestCol() = default;

  struct TestMsg : vt::CollectionMessage<TestCol> { };

  void makeHandles(TestMsg*) {
    auto proxy = this->getCollectionProxy();
    auto idx = this->getIndex();
    handle_ = proxy.template makeHandleRDMA<T>(idx, 8, true);
  }

  void setupData(TestMsg* msg) {
    auto idx = this->getIndex();
    handle_.modifyExclusive([&](T* t, std::size_t count) {
      for (int i = 0; i < 8; i++) {
        t[i] = idx.x() * 100 + idx.y();
      }
    });
  }

  void testData(TestMsg*) {
    auto idx = this->getIndex();
    auto next_x = idx.x() + 1 < 8 ? idx.x() + 1 : 0;
    vt::Index2D next(next_x, idx.y());
    auto ptr = std::make_unique<T[]>(8);
    handle_.get(next, ptr.get(), 8, 0, vt::Lock::Shared);
    for (int i = 0; i < 8; i++) {
      EXPECT_EQ(ptr[i], next_x * 100 + idx.y());
    }
  }

  void migrateObjs(TestMsg*) {
    auto idx = this->getIndex();
    if (idx.y() > 1) {
      auto node = vt::theContext()->getNode();
      auto num = vt::theContext()->getNumNodes();
      auto next = node + 1 < num ? node + 1 : 0;
      this->migrate(next);
    }
  }

  void runLBHooksForRDMA(TestMsg*) {
    if (not triggered_lb) {
      triggered_lb = true;
      //fmt::print("{}: run post migration hooks\n", theContext()->getNode());
      vt::thePhase()->runHooksManual(vt::phase::PhaseHook::EndPostMigration);
    }
  }

  void checkDataAfterMigrate(TestMsg*) {
    auto idx = this->getIndex();
    auto next_x = idx.x() + 1 < 8 ? idx.x() + 1 : 0;
    vt::Index2D next(next_x, idx.y());
    auto ptr = std::make_unique<T[]>(8);
    handle_.get(next, ptr.get(), 8, 0, vt::Lock::Shared);
    for (int i = 0; i < 8; i++) {
      EXPECT_EQ(ptr[i], next_x * 100 + idx.y());
    }
  }

  void destroyHandles(TestMsg*) {
    auto proxy = this->getCollectionProxy();
    proxy.destroyHandleRDMA(handle_);
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    vt::Collection<TestCol<T>, vt::Index2D>::serialize(s);
    s | handle_;
  }

private:
  vt::HandleRDMA<T, vt::Index2D> handle_;
};

template <typename T>
struct TestRDMAHandleCollection : TestParallelHarness { };

TYPED_TEST_SUITE_P(TestRDMAHandleCollection);

TYPED_TEST_P(TestRDMAHandleCollection, test_rdma_handle_collection_1) {
  SET_MAX_NUM_NODES_CONSTRAINT(CMAKE_DETECTED_MAX_NUM_NODES);

  using T = TypeParam;
  using ColType = TestCol<T>;
  using MsgType = typename ColType::TestMsg;

  triggered_lb = false;

  CollectionProxy<TestCol<T>, Index2D> proxy;

  runInEpochCollective([&]{
    auto range = vt::Index2D(8,8);
    proxy = theCollection()->constructCollective<ColType>(range);
  });

  runInEpochCollective([=]{
    proxy.template broadcastCollective<MsgType, &ColType::makeHandles>();
  });

  runInEpochCollective([=]{
    proxy.template broadcastCollective<MsgType, &ColType::setupData>();
  });

  runInEpochCollective([=]{
    proxy.template broadcastCollective<MsgType, &ColType::testData>();
  });

  runInEpochCollective([=]{
    proxy.template broadcastCollective<MsgType, &ColType::migrateObjs>();
  });

  runInEpochCollective([=]{
    proxy.template broadcastCollective<MsgType, &ColType::runLBHooksForRDMA>();
  });

  runInEpochCollective([=]{
    proxy.template broadcastCollective<MsgType, &ColType::checkDataAfterMigrate>();
  });

  runInEpochCollective([=]{
    proxy.template broadcastCollective<MsgType, &ColType::destroyHandles>();
  });
}

using RDMACollectionTestTypes = testing::Types<
  int,
  double
>;

REGISTER_TYPED_TEST_SUITE_P(
  TestRDMAHandleCollection,
  test_rdma_handle_collection_1
);

INSTANTIATE_TYPED_TEST_SUITE_P(
  test_rdma_handle_collection, TestRDMAHandleCollection, RDMACollectionTestTypes,
  DEFAULT_NAME_GEN
);

}}} /* end namespace vt::tests::unit */
