/*
//@HEADER
// *****************************************************************************
//
//                         test_checkpoint.extended.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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
#include "vt/objgroup/manager.h"

#include <memory>

namespace vt { namespace tests { namespace unit {

static constexpr std::size_t data1_len = 1024;
static constexpr std::size_t data2_len = 64;

struct TestObj {

  struct NullMsg : vt::Message {};

  void init(NullMsg*) {
    auto idx = vt::theContext()->getNode();

    iter = 1;
    data1.resize(data1_len);
    data2.resize(data2_len);

    for (std::size_t i = 0; i < data1_len; i++) {
      data1[i] = i + idx * 24;
    }
    for (std::size_t i = 0; i < data2_len; i++) {
      data2[i] = i + idx * 124;
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
    auto idx = vt::theContext()->getNode();

    EXPECT_EQ(iter, 6);
    EXPECT_EQ(data1.size(), data1_len);
    EXPECT_EQ(data2.size(), data2_len);

    for (std::size_t i = 0; i < data1_len; i++) {
      EXPECT_EQ(data1[i], 5 + i + idx * 24);
    }
    for (std::size_t i = 0; i < data2_len; i++) {
      EXPECT_EQ(data2[i], 5 + i + idx * 124);
    }

    EXPECT_NE(token, nullptr);
    EXPECT_EQ(*token, 129);
    EXPECT_EQ(final_node, theContext()->getNode());
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | iter;
    s | data1 | data2;
    if (s.isUnpacking()) {
      token = std::make_shared<int>();
    }
    bool is_null = token == nullptr;
    s | is_null;
    if (not is_null) {
      s | *token;
    }
    s | final_node;
  }

  int iter = 0;
  std::vector<double> data1, data2;
  std::shared_ptr<int> token;
  NodeType final_node = vt::theContext()->getNode();
};

using TestObjGroupCheckpoint = TestParallelHarness;

TEST_F(TestObjGroupCheckpoint, test_objgroup_checkpoint) {
  using namespace magistrate;
  using vt::vrt::CheckpointTrait;
  using ProxyType = vt::objgroup::proxy::Proxy<TestObj>;

  std::unique_ptr<SerializedInfo> checkpoint;

  auto this_node = theContext()->getNode();

  std::string const expected_label{"test_objgroup_checkpoint"};

  {
    ProxyType proxy = vt::theObjGroup()->makeCollective<TestObj>(
      expected_label
    );

    vt::runInEpochCollective([&]{
      if (this_node == 0) {
        proxy.broadcast<TestObj::NullMsg,&TestObj::init>();
      }
    });

    for (int i = 0; i < 5; i++) {
      vt::runInEpochCollective([&]{
        if (this_node == 0) {
          proxy.template broadcast<TestObj::NullMsg,&TestObj::doIter>();
        }
      });
    }


    checkpoint = serialize<ProxyType, CheckpointTrait>(proxy);

    // Wait for all checkpoints to complete
    vt::theCollective()->barrier();

    // Null the token to ensure we don't end up getting the same instance
    vt::runInEpochCollective([&]{
      if (this_node == 0) {
        proxy.broadcast<TestObj::NullMsg,&TestObj::nullToken>();
      }
    });

    // Destroy the collection
    vt::runInEpochCollective([&]{
      if (this_node == 0) {
        proxy.destroyCollective();
      }
    });

    vt::theCollective()->barrier();
  }

  {
    ProxyType proxy = vt::theObjGroup()->makeCollective<TestObj>(
      expected_label
    );

    deserializeInPlace<ProxyType, CheckpointTrait>(
      checkpoint->getBuffer(), &proxy
    );

    // Restoration should be done now
    vt::theCollective()->barrier();

    runInEpochCollective([&] {
      auto const got_label = vt::theObjGroup()->getLabel(proxy);
      EXPECT_EQ(got_label, expected_label);

      if (this_node == 0) {
        proxy.broadcast<TestObj::NullMsg, &TestObj::verify>();
      }
    });

    runInEpochCollective([&]{
      proxy.destroyCollective();
    });
  }
}

}}}
