/*
//@HEADER
// *****************************************************************************
//
//                               test_destroy.cc
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

#include "test_parallel_harness.h"
#include "test_collection_common.h"
#include "data_message.h"

#include "vt/transport.h"

#include <cstdint>

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::collective;
using namespace vt::tests::unit;

struct WorkMsg;

static int32_t num_destroyed = 0;

struct DestroyTest : Collection<DestroyTest,Index1D> {
  DestroyTest() = default;

  virtual ~DestroyTest() {
    // ::fmt::print("destroying collection: idx={}\n", getIndex().x());
    num_destroyed++;
  }

  static void work(WorkMsg* msg, DestroyTest* col);
};

struct WorkMsg : CollectionMessage<DestroyTest> {};
using ColProxyType = CollectionIndexProxy<DestroyTest,Index1D>;
struct CollReduceMsg : collective::ReduceTMsg<NoneType> {
  CollReduceMsg() = default;
  explicit CollReduceMsg(ColProxyType in_proxy)
    : proxy_(in_proxy)
  {}
  ColProxyType proxy_ = {};
};

struct FinishedWork {
  void operator()(CollReduceMsg* msg) {
    msg->proxy_.destroy();
  }
};

/*static*/ void DestroyTest::work(WorkMsg* msg, DestroyTest* col) {
  auto proxy = col->getCollectionProxy();
  // ::fmt::print("work idx={}, proxy={:x}\n", col->getIndex(), proxy.getProxy());
  auto reduce_msg = makeMessage<CollReduceMsg>(proxy);
  theCollection()->reduceMsg<
    DestroyTest,
    CollReduceMsg,
    CollReduceMsg::template msgHandler<
      CollReduceMsg, collective::PlusOp<collective::NoneType>, FinishedWork
    >
  >(proxy,reduce_msg.get());
}

struct TestDestroy : TestParallelHarness { };

static constexpr int32_t const num_elms_per_node = 8;

TEST_F(TestDestroy, test_destroy_1) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();
  if (this_node == 0) {
    auto const& range = Index1D(num_nodes * num_elms_per_node);
    auto proxy = theCollection()->construct<DestroyTest>(range);
    auto msg = makeMessage<WorkMsg>();
    // ::fmt::print("broadcasting proxy={:x}\n", proxy.getProxy());
    proxy.broadcast<WorkMsg,DestroyTest::work>(msg.get());
  }
  theTerm()->addAction([]{
    // ::fmt::print("num destroyed={}\n", num_destroyed);
    // Relies on default mapping equally distributing
    EXPECT_EQ(num_destroyed, num_elms_per_node);
  });
}

}}} // end namespace vt::tests::unit
