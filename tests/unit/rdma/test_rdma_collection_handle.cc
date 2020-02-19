/*
//@HEADER
// *****************************************************************************
//
//                       test_rdma_collection_handle.cc
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

bool triggered_lb = false;

template <typename T>
struct TestCol : vt::Collection<TestCol<T>, vt::Index2D> {
  TestCol() = default;

  struct TestMsg : vt::CollectionMessage<TestCol> {
    TestMsg() = default;
    explicit TestMsg(EpochType in_epoch)
      : migrate_epoch_(in_epoch)
    { }
    EpochType migrate_epoch_;
  };
  struct ReduceMsg : vt::collective::ReduceNoneMsg {};

  void initialize(TestMsg* mm) {
    auto idx = this->getIndex();
    auto proxy = this->getCollectionProxy();
    proxy[idx].template send<
      typename TestCol<T>::TestMsg, &TestCol<T>::initialize2
    >();
    migrate_epoch_ = mm->migrate_epoch_;
  }

  void initialize2(TestMsg*) {
    auto proxy = this->getCollectionProxy();
    auto idx = this->getIndex();
    handle_ = proxy.template makeHandleRDMA<int>(this->getIndex(), 8, true);
    do vt::runScheduler(); while (not handle_.ready());
    handle_.modifyExclusive([&](int* t) {
      for (int i = 0; i < 8; i++) {
        t[i] = idx.x() * 100 + idx.y();
      }
    });
    auto cb = theCB()->makeBcast<
      TestCol<T>,ReduceMsg,&TestCol<T>::afterDataInit
    >(proxy);
    auto rmsg = makeMessage<ReduceMsg>();
    proxy.reduce(rmsg.get(),cb);
  }

  void afterDataInit(ReduceMsg*) {
    auto idx = this->getIndex();
    auto next_x = idx.x() + 1 < 8 ? idx.x() + 1 : 0;
    vt::Index2D next(next_x, idx.y());
    auto ptr = std::make_unique<T[]>(8);
    handle_.get(next, &ptr[0], 8, 0, vt::Lock::Shared);
    for (int i = 0; i < 8; i++) {
      EXPECT_EQ(ptr[i], next_x * 100 + idx.y());
    }
    auto proxy = this->getCollectionProxy();
    auto cb = theCB()->makeBcast<
      TestCol<T>,ReduceMsg,&TestCol<T>::afterDataCheck
    >(proxy);
    auto rmsg = makeMessage<ReduceMsg>();
    proxy.reduce(rmsg.get(),cb);
  }

  void afterDataCheck(ReduceMsg*) {
    auto idx = this->getIndex();
    theMsg()->pushEpoch(migrate_epoch_);
    if (idx.x() == 0 and idx.y() == 0) {
      theTerm()->consume(migrate_epoch_);
    }
    if (idx.y() > 1) {
      auto node = vt::theContext()->getNode();
      auto num = vt::theContext()->getNumNodes();
      auto next = node + 1 < num ? node + 1 : 0;
      this->migrate(next);
    }
    theMsg()->popEpoch(migrate_epoch_);
  }

  void afterMigrate(TestMsg*) {
    auto idx = this->getIndex();
    auto proxy = this->getCollectionProxy();
    proxy[idx].template send<
      typename TestCol<T>::TestMsg, &TestCol<T>::afterMigratePost
    >();
  }

  void afterMigratePost(TestMsg*) {
    if (not triggered_lb) {
      triggered_lb = true;
      auto lb_proxy = vt::vrt::collection::balance::LBManager::getProxy();
      //fmt::print("{}: triggering listeners\n", theContext()->getNode());
      lb_proxy.get()->triggerListeners(0);
    }
    auto proxy = this->getCollectionProxy();
    auto cb = theCB()->makeBcast<
      TestCol<T>,ReduceMsg,&TestCol<T>::afterMigrateCheck
    >(proxy);
    auto rmsg = makeMessage<ReduceMsg>();
    proxy.reduce(rmsg.get(),cb);
  }

  void afterMigrateCheck(ReduceMsg*) {
    auto idx = this->getIndex();
    auto next_x = idx.x() + 1 < 8 ? idx.x() + 1 : 0;
    vt::Index2D next(next_x, idx.y());
    auto ptr = std::make_unique<T[]>(8);
    handle_.get(next, &ptr[0], 8, 0, vt::Lock::Shared);
    for (int i = 0; i < 8; i++) {
      EXPECT_EQ(ptr[i], next_x * 100 + idx.y());
    }
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    vt::Collection<TestCol<T>, vt::Index2D>::serialize(s);
    s | handle_;
    s | migrate_epoch_;
  }

private:
  vt::Handle<T, vt::rdma::HandleEnum::StaticSize, vt::Index2D> handle_;
  vt::EpochType migrate_epoch_;
};

template <typename T>
struct TestRDMAHandleCollection : TestParallelHarness { };

TYPED_TEST_CASE_P(TestRDMAHandleCollection);

TYPED_TEST_P(TestRDMAHandleCollection, test_rdma_handle_collection_1) {
  using T = TypeParam;
  triggered_lb = false;

  auto range = vt::Index2D(8,8);
  auto proxy = theCollection()->constructCollective<TestCol<T>>(
    range, [](vt::Index2D idx){
      return std::make_unique<TestCol<T>>();
    }
  );

  auto migrate_epoch = theTerm()->makeEpochCollective();
  if (theContext()->getNode() == 0) {
    theTerm()->produce(migrate_epoch);
    proxy.template broadcast<
      typename TestCol<T>::TestMsg, &TestCol<T>::initialize
    >(migrate_epoch);
    theTerm()->addAction(migrate_epoch,[=]{
      proxy.template broadcast<
        typename TestCol<T>::TestMsg, &TestCol<T>::afterMigrate
      >();
    });
  }
  theTerm()->finishedEpoch(migrate_epoch);

  do vt::runScheduler(); while (not vt::rt->isTerminated());
}

using RDMACollectionTestTypes = testing::Types<
  int// ,
  // double,
  // float,
  // int32_t,
  // int64_t,
  // uint64_t,
  // int64_t,
  // int16_t,
  // uint16_t
>;

REGISTER_TYPED_TEST_CASE_P(
  TestRDMAHandleCollection,
  test_rdma_handle_collection_1
  // test_rdma_handle_set_2,
  // test_rdma_handle_set_3,
  // test_rdma_handle_set_4,
  // test_rdma_handle_set_5
);

INSTANTIATE_TYPED_TEST_CASE_P(
  test_rdma_handle_collection, TestRDMAHandleCollection, RDMACollectionTestTypes
);

}}} /* end namespace vt::tests::unit */
