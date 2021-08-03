/*
//@HEADER
// *****************************************************************************
//
//                      test_collection_group_recreate.cc
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

#include <vt/transport.h>

#include <gtest/gtest.h>

#include "test_parallel_harness.h"

namespace vt { namespace tests { namespace unit {

struct MyReduceMsg : vt::collective::ReduceTMsg<int> {
  explicit MyReduceMsg(int const in_num)
    : vt::collective::ReduceTMsg<int>(in_num)
  { }
};

struct MyCol : vt::Collection<MyCol,vt::Index1D> {
  struct TestMsg : vt::CollectionMessage<MyCol> {
    explicit TestMsg(vt::Callback<MyReduceMsg> in_cb) : cb(in_cb) { }
    vt::Callback<MyReduceMsg> cb;
  };
  struct TestMsg2 : vt::CollectionMessage<MyCol> {};

  void doWork(TestMsg2* msg) {}

  void doReduce(TestMsg* msg) {
    auto const proxy = getCollectionProxy();
    auto reduce_msg = makeMessage<MyReduceMsg>(getIndex().x());
    proxy.reduce<collective::PlusOp<int>>(reduce_msg.get(),msg->cb);
  }
};

struct TestCollectionGroupRecreate : TestParallelHarness { };

TEST_F(TestCollectionGroupRecreate, test_collection_group_recreate_1) {
  auto const this_node = theContext()->getNode();
  auto const num_nodes = theContext()->getNumNodes();

  // Create a range that only hits half of the node---this requires a group for
  // the reduction to finish properly
  auto const range = Index1D(std::max(num_nodes / 2, 1));
  auto const range2 = Index1D(num_nodes * 8);
  auto const proxy = theCollection()->constructCollective<MyCol>(
    range, [](vt::Index1D) {
      return std::make_unique<MyCol>();
    }
  );
  auto const proxy2 = theCollection()->constructCollective<MyCol>(
    range2, [](vt::Index1D) {
      return std::make_unique<MyCol>();
    }
  );

  int cb_counter = 0;

  /// Broadcast to do a reduction over the current group
  vt::runInEpochCollective([&]{
    if (this_node == 0) {
      proxy2.broadcast<MyCol::TestMsg2,&MyCol::doWork>();
      auto cb = vt::theCB()->makeFunc<MyReduceMsg>(
        vt::pipe::LifetimeEnum::Once, [&cb_counter](MyReduceMsg* m) {
          cb_counter++;
          fmt::print("at root: final num={}\n", m->getVal());
        }
      );
      proxy.broadcast<MyCol::TestMsg,&MyCol::doReduce>(cb);
    }
  });

  if (this_node == 0) {
    EXPECT_EQ(cb_counter, 1);
  }

  /// Run RotateLB to make the previous group invalid!
  vt::theConfig()->vt_lb = true;
  vt::theConfig()->vt_lb_name = "RotateLB";
  vt::theConfig()->vt_lb_interval = 1;

  vt::theCollective()->barrier();

  vt::runInEpochCollective([&]{
    vt::thePhase()->nextPhaseCollective();
  });

  // Try to do another reduction; should fail if group is not setup correctly
  vt::runInEpochCollective([&]{
    if (this_node == 0) {
      proxy2.broadcast<MyCol::TestMsg2,&MyCol::doWork>();
      auto cb = vt::theCB()->makeFunc<MyReduceMsg>(
        vt::pipe::LifetimeEnum::Once, [&cb_counter](MyReduceMsg* m) {
          cb_counter++;
          fmt::print("at root: final num={}\n", m->getVal());
        }
      );
      proxy.broadcast<MyCol::TestMsg,&MyCol::doReduce>(cb);
    }
  });

  if (this_node == 0) {
    EXPECT_EQ(cb_counter, 2);
  }

  vt::runInEpochCollective([&]{
    vt::thePhase()->nextPhaseCollective();
  });

  // Try to do another reduction; should fail if group is not setup correctly
  vt::runInEpochCollective([&]{
    if (this_node == 0) {
      proxy2.broadcast<MyCol::TestMsg2,&MyCol::doWork>();
      auto cb = vt::theCB()->makeFunc<MyReduceMsg>(
        vt::pipe::LifetimeEnum::Once, [&cb_counter](MyReduceMsg* m) {
          cb_counter++;
          fmt::print("at root: final num={}\n", m->getVal());
        }
      );
      proxy.broadcast<MyCol::TestMsg,&MyCol::doReduce>(cb);
    }
  });

  if (this_node == 0) {
    EXPECT_EQ(cb_counter, 3);
  }

}

}}} // end namespace vt::tests::unit
