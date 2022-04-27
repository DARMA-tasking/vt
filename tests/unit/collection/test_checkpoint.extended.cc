/*
//@HEADER
// *****************************************************************************
//
//                         test_checkpoint.extended.cc
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

  void saveNode(NullMsg*) {
    final_node = theContext()->getNode();
  }

  void checkNode(NullMsg*) {
    EXPECT_EQ(final_node, theContext()->getNode());
  }

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
  NodeType final_node = uninitialized_destination;
};

using TestCheckpoint = TestParallelHarness;

static constexpr int32_t const num_elms = 8;

TEST_F(TestCheckpoint, test_checkpoint_1) {
  auto this_node = theContext()->getNode();
  auto num_nodes = static_cast<int32_t>(theContext()->getNumNodes());

  auto range = vt::Index3D(num_nodes, num_elms, 4);
  auto checkpoint_name = "test_checkpoint_dir";

  {
    auto proxy = vt::theCollection()->constructCollective<TestCol>(range);

    vt::runInEpochCollective([&]{
      if (this_node == 0) {
        proxy.broadcast<TestCol::NullMsg,&TestCol::init>();
      }
    });

    for (int i = 0; i < 5; i++) {
      vt::runInEpochCollective([&]{
        if (this_node == 0) {
          proxy.template broadcast<TestCol::NullMsg,&TestCol::doIter>();
        }
      });
    }

    vt::theCollection()->checkpointToFile(proxy, checkpoint_name);

    // Wait for all checkpoints to complete
    vt::theCollective()->barrier();

    // Null the token to ensure we don't end up getting the same instance
    vt::runInEpochCollective([&]{
      if (this_node == 0) {
        proxy.broadcast<TestCol::NullMsg,&TestCol::nullToken>();
      }
    });

    vt::thePhase()->nextPhaseCollective();

    // Destroy the collection
    vt::runInEpochCollective([&]{
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

TEST_F(TestCheckpoint, test_checkpoint_in_place_2) {
  auto this_node = theContext()->getNode();
  auto num_nodes = static_cast<int32_t>(theContext()->getNumNodes());

  auto range = vt::Index3D(num_nodes, num_elms, 4);
  auto checkpoint_name = "test_checkpoint_dir";
  auto proxy = vt::theCollection()->constructCollective<TestCol>(range);

  theConfig()->vt_lb = true;
  theConfig()->vt_lb_name = "TemperedLB";

  vt::runInEpochCollective([&]{
    if (this_node == 0) {
      proxy.broadcast<TestCol::NullMsg,&TestCol::init>();
    }
  });

  for (int i = 0; i < 5; i++) {
    vt::runInEpochCollective([&]{
      if (this_node == 0) {
        proxy.template broadcast<TestCol::NullMsg,&TestCol::doIter>();
      }
    });

    vt::thePhase()->nextPhaseCollective();
  }

  vt_print(gen, "checkpointToFile\n");
  vt::theCollection()->checkpointToFile(proxy, checkpoint_name);

  // Wait for all checkpoints to complete
  vt::theCollective()->barrier();

  // Do more work after the checkpoint
  vt::runInEpochCollective([&]{
    if (this_node == 0) {
      proxy.template broadcast<TestCol::NullMsg,&TestCol::doIter>();
    }
  });

  // Run the LB one more time so we might get a different distribution
  vt::thePhase()->nextPhaseCollective();

  vt::theCollective()->barrier();

  // Now, restore from the previous distribution
  vt_print(gen, "restoreFromFileInPlace\n");
  vt::theCollection()->restoreFromFileInPlace<TestCol>(
    proxy, range, checkpoint_name
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

TEST_F(TestCheckpoint, test_checkpoint_in_place_3) {
  auto this_node = theContext()->getNode();
  auto num_nodes = static_cast<int32_t>(theContext()->getNumNodes());

  auto range = vt::Index3D(num_nodes, num_elms, 4);
  auto checkpoint_name = "test_checkpoint_dir_2";
  auto proxy = vt::theCollection()->constructCollective<TestCol>(range);

  theConfig()->vt_lb = true;
  theConfig()->vt_lb_name = "TemperedLB";

  vt::runInEpochCollective([&]{
    if (this_node == 0) {
      proxy.broadcast<TestCol::NullMsg,&TestCol::init>();
    }
  });

  for (int i = 0; i < 5; i++) {
    vt::runInEpochCollective([&]{
      if (this_node == 0) {
        proxy.template broadcast<TestCol::NullMsg,&TestCol::doIter>();
      }
    });

    vt::thePhase()->nextPhaseCollective();
  }

  vt::runInEpochCollective([&]{
    if (this_node == 0) {
      proxy.broadcast<TestCol::NullMsg,&TestCol::saveNode>();
    }
  });

  vt::runInEpochCollective([&]{
    vt_print(gen, "checkpointToFile\n");
    vt::theCollection()->checkpointToFile(proxy, checkpoint_name);
  });

  vt::runInEpochCollective([&]{
    if (this_node == 0) {
      proxy.destroy();
    }
  });

  auto proxy_new = vt::theCollection()->constructCollective<TestCol>(range);

  vt::runInEpochCollective([&]{
    // Now, restore from the previous distribution
    vt_print(gen, "restoreFromFileInPlace\n");
    vt::theCollection()->restoreFromFileInPlace<TestCol>(
      proxy_new, range, checkpoint_name
    );
  });

  // Do more work after the checkpoint
  vt::runInEpochCollective([&]{
    proxy_new.broadcastCollective<TestCol::NullMsg,&TestCol::checkNode>();
  });
}


// Goals for Test:
//  1. Create a collection and get it into a state where there is a rank with zero elements
//      (either by  (A) setting it up that way initially or (B) migrating everything off one rank)
//  2. Checkpoint the collection
//  3. Restore the collection and validate it

vt::NodeType map(vt::Index3D* idx, vt::Index3D* max_idx, vt::NodeType num_nodes) {
  return (idx->x() % (num_nodes-1))+1;
}

TEST_F(TestCheckpoint, test_checkpoint_no_elements_on_root_rank) {
  SET_MIN_NUM_NODES_CONSTRAINT(2);

  auto this_node = vt::theContext()->getNode();
  auto num_nodes = static_cast<int32_t>(theContext()->getNumNodes());

  auto range = vt::Index3D(num_nodes, num_elms, 4);
  auto checkpoint_name = "test_null_elm_checkpoint_dir";

  {
    auto proxy = vt::makeCollection<TestCol>()
      .bounds(range)
      .mapperFunc<map>()
      .bulkInsert()
      .wait();

    vt::runInEpochCollective([&]{
      if (this_node == 0) {
        proxy.broadcast<TestCol::NullMsg, &TestCol::init>();
      }
    });
    //this number of iterations is expected in the verify member function
    for (int i = 0; i < 5; i++) {
      vt::runInEpochCollective([&]{
        if(this_node == 0) {
          proxy.broadcast<TestCol::NullMsg, &TestCol::doIter>();
        }
      });
    }
    //verify that root node has no elements, by construction with map
    if(this_node == 0) {
      auto local_set = theCollection()->getLocalIndices(proxy);
      EXPECT_EQ(local_set.size(), 0);
    }

    vt_print(gen, "checkpointToFile\n");
    vt::theCollection()->checkpointToFile(proxy, checkpoint_name);

    // Wait for all checkpoints to complete
    vt::theCollective()->barrier();

    // Destroy the collection
    vt::runInEpochCollective([&]{
      if (this_node == 0) {
        proxy.destroy();
      }
    });
      // Wait for all checkpoints to complete
    vt::theCollective()->barrier();
  }

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

}

}}} // end namespace vt::tests::unit
