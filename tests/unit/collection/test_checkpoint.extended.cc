/*
//@HEADER
// *****************************************************************************
//
//                         test_checkpoint.extended.cc
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

#include <memory>

namespace vt { namespace tests { namespace unit {

static constexpr std::size_t data1_len = 1024;
static constexpr std::size_t data2_len = 64;

static std::size_t counter = 0;

struct TestCol : vt::Collection<TestCol,vt::Index3D> {

  TestCol() {
    // fmt::print("{} ctor\n", theContext()->getNode());
    counter++;
  }
  TestCol(TestCol&& other)
    : iter(other.iter),
      data1(std::move(other.data1)),
      data2(std::move(other.data2)),
      token(other.token)
  {
    // fmt::print("{} move ctor\n", theContext()->getNode());
    counter++;
  }
  TestCol(TestCol const& other)
    : iter(other.iter),
      data1(other.data1),
      data2(other.data2),
      token(other.token)
  {
    // fmt::print("{} copy ctor\n", theContext()->getNode());
    counter++;
  }

  virtual ~TestCol() {
    // fmt::print("{} destroying\n", theContext()->getNode());
    counter--;
  }

  struct NullMsg : vt::CollectionMessage<TestCol> {};

  void init(NullMsg*) {
    auto idx = getIndex();

    iter = 1;
    data1.resize(data1_len);
    data2.resize(data2_len);

    for (std::size_t i = 0; i < data1_len; i++) {
      data1[i] = i + idx.x() * 24 + idx.y() * 48 + idx.z();
    }
    for (std::size_t i = 0; i < data2_len; i++) {
      data2[i] = i + idx.x() * 124 + idx.y() * 148 + idx.z();
    }

    token = std::make_shared<int>(129);
  }

  void doIter(NullMsg*) {
    iter++;
    for (auto& elm : data1) { elm += 1.; }
    for (auto& elm : data2) { elm += 1.; }
  }

  void nullToken(NullMsg*) {
    token = nullptr;
  }

  void verify(NullMsg*) {
    auto idx = getIndex();

    EXPECT_EQ(iter, 6);
    EXPECT_EQ(data1.size(), data1_len);
    EXPECT_EQ(data2.size(), data2_len);

    for (std::size_t i = 0; i < data1_len; i++) {
      EXPECT_EQ(data1[i], 5 + i + idx.x() * 24 + idx.y() * 48 + idx.z());
    }
    for (std::size_t i = 0; i < data2_len; i++) {
      EXPECT_EQ(data2[i], 5 + i + idx.x() * 124 + idx.y() * 148 + idx.z());
    }

    EXPECT_NE(token, nullptr);
    EXPECT_EQ(*token, 129);
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    vt::Collection<TestCol,vt::Index3D>::serialize(s);
    s | iter;
    s | data1 | data2;
    if (s.isUnpacking()) {
      token = std::make_shared<int>();
    }
    s | *token;
  }

  int iter = 0;
  std::vector<double> data1, data2;
  std::shared_ptr<int> token;
};

static void runInEpoch(std::function<void()> fn) {
  vt::EpochType ep = vt::theTerm()->makeEpochCollective();
  vt::theMsg()->pushEpoch(ep);
  fn();
  vt::theMsg()->popEpoch(ep);
  vt::theTerm()->finishedEpoch(ep);
  bool done = false;
  vt::runInEpochCollective([&]{ done = true; });
  vt::theSched()->runSchedulerWhile([&] { return not done; });
}

using TestCheckpoint = TestParallelHarness;

static constexpr int32_t const num_elms = 8;

TEST_F(TestCheckpoint, test_checkpoint_1) {
  auto this_node = theContext()->getNode();
  auto num_nodes = static_cast<int32_t>(theContext()->getNumNodes());

  auto range = vt::Index3D(num_nodes, num_elms, 4);
  auto checkpoint_name = "test_checkpoint_dir";

  {
    auto proxy = vt::theCollection()->constructCollective<TestCol>(
      range, [](vt::Index3D){
        return std::make_unique<TestCol>();
      }
    );

    runInEpoch([&]{
      if (this_node == 0) {
        proxy.broadcast<TestCol::NullMsg,&TestCol::init>();
      }
    });

    for (int i = 0; i < 5; i++) {
      runInEpoch([&]{
        if (this_node == 0) {
          proxy.template broadcast<TestCol::NullMsg,&TestCol::doIter>();
        }
      });
    }

    vt::theCollection()->checkpointToFile(proxy, checkpoint_name);

    // Wait for all checkpoints to complete
    vt::theCollective()->barrier();

    // Null the token to ensure we don't end up getting the same instance
    runInEpoch([&]{
      if (this_node == 0) {
        proxy.broadcast<TestCol::NullMsg,&TestCol::nullToken>();
      }
    });

    // Destroy the collection
    runInEpoch([&]{
      if (this_node == 0) {
        proxy.destroy();
      }
    });

    vt::theCollective()->barrier();
  }

  {
    auto proxy = vt::theCollection()->restoreFromFile<TestCol>(
      range, checkpoint_name
    );

    // Restoration should be done now
    vt::theCollective()->barrier();

    runInEpochCollective([&]{
      if (this_node == 0) {
        proxy.broadcast<TestCol::NullMsg,&TestCol::verify>();
      }
    });

    runInEpochCollective([&]{
      if (this_node == 0) {
        proxy.destroy();
      }
    });

    // Ensure that all elements were properly destroyed
    EXPECT_EQ(counter, 0);
  }

}

}}} // end namespace vt::tests::unit
