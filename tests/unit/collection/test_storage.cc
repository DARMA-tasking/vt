/*
//@HEADER
// *****************************************************************************
//
//                               test_storage.cc
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

#include "test_parallel_harness.h"

#include <vt/transport.h>

#include <gtest/gtest.h>

namespace vt { namespace tests { namespace unit { namespace storage {

struct TestData {
  TestData() = default;

  TestData(int64_t in_a, float in_b)
    : a(in_a),
      b(in_b)
  { }

  int64_t a = 0;
  float b = 0.;
};

struct TestCol : Collection<TestCol, Index1D> {

  struct TestMsg : CollectionMessage<TestCol> { };

  void testHandler(TestMsg* msg) {
    this->valInsert("hello", this->getIndex().x());
    this->valRemove("hello");
    this->valInsert("hello", this->getIndex().x());

    this->valInsert("vec", std::vector<int>{5,4,1,7,3});
    this->valInsert("test", TestData(10, 20.5f));

    auto a = std::make_unique<int>(1000);
    this->valInsert("up", std::move(a));

    testHandlerValues(msg);
  }

  void testHandlerValues(TestMsg* msg) {
    EXPECT_EQ(this->valGet<int>("hello"), this->getIndex().x());

    std::vector<int> in_vec{5,4,1,7,3};
    auto vec = this->valGet<std::vector<int>>("vec");
    for (std::size_t i = 0; i < in_vec.size(); i++) {
      EXPECT_EQ(vec[i], in_vec[i]);
    }

    auto v = this->valGet<TestData>("test");
    EXPECT_EQ(v.a, 10);
    EXPECT_EQ(v.b, 20.5f);

    auto& up = this->valGet<std::unique_ptr<int>>("up");
    EXPECT_EQ(*up, 1000);
  }

};

struct TestCollectionStorage : TestParallelHarness {
#if vt_check_enabled(lblite)
  void addAdditionalArgs() override {
    static char vt_lb[]{"--vt_lb"};
    static char vt_lb_name[]{"--vt_lb_name=RotateLB"};
    addArgs(vt_lb, vt_lb_name);
  }
#endif
};

TEST_F(TestCollectionStorage, test_collection_storage_1) {
  auto const num_nodes = theContext()->getNumNodes();
  auto const num_elms = Index1D{num_nodes*16};

  using MsgType = typename TestCol::TestMsg;

  auto proxy = theCollection()->constructCollective<TestCol>(num_elms);
  proxy.broadcastCollective<MsgType, &TestCol::testHandler>();

  // Go to the next phase
  vt::thePhase()->nextPhaseCollective();
  proxy.broadcastCollective<MsgType, &TestCol::testHandlerValues>();

  // Go to the next phase
  vt::thePhase()->nextPhaseCollective();
  proxy.broadcastCollective<MsgType, &TestCol::testHandlerValues>();
}

}}}} // end namespace vt::tests::unit::storage
