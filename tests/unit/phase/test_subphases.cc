/*
//@HEADER
// *****************************************************************************
//
//                             test_subphases.cc
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
#include <vt/phase/subphase/subphase_bits.h>
#include <gtest/gtest.h>

#include "test_parallel_harness.h"

namespace vt { namespace tests { namespace unit {

using TestSubphases = TestParallelHarness;

TEST_F(TestSubphases, test_subphase_bits) {
  using phase::subphase::SubphaseBits;

  SubphaseType const bits = 0xFFFFFFFFFFFFFFFF;

  // Check that bit mixing works properly with the same ID
  bool const collective = true;
  auto cid = SubphaseBits::makeID(collective, 0, bits);
  auto rid = SubphaseBits::makeID(not collective, 0, bits);
  EXPECT_NE(cid, rid);

  // Check that rooted subphase bits with different nodes are distinct
  auto rid1 = SubphaseBits::makeID(not collective, 1, bits);
  EXPECT_NE(rid, rid1);
}

struct SubphaseIDTestMsg : Message {
  explicit SubphaseIDTestMsg(SubphaseType in_id)
    : id_(in_id)
  { }
  SubphaseType id_ = 0;
};

static SubphaseType expected_id = 0;

struct SubphaseHandler {
  void operator()(SubphaseIDTestMsg* msg) const {
    EXPECT_EQ(expected_id, msg->id_);
  }
};

TEST_F(TestSubphases, test_subphase_rooted_resolution) {
  EXPECT_FALSE(thePhase()->isPendingSubphaseResolution());

  bool done = false;

  std::string label1{"my-test-label1"};
  SubphaseType id1 = 0;

  vt_print(gen, "registering a rooted subphase\n");

  // All nodes register the same rooted subphase label and should end up with
  // the same ID from the broker
  thePhase()->registerRootedSubphase(label1, [&done,&id1](SubphaseType id) {
    vt_print(gen, "rooted subphase generated\n");
    id1 = id;
    expected_id = id1;
    done = true;
  });

  // Start the next phase, which would require all resolution is complete
  thePhase()->nextPhaseCollective();

  EXPECT_TRUE(done);
  EXPECT_NE(id1, 0);

  auto const this_node = theContext()->getNode();
  if (this_node != 0) {
    NodeType const send_node = 0;
    auto msg = makeMessage<SubphaseIDTestMsg>(id1);
    theMsg()->sendMsg<SubphaseHandler, SubphaseIDTestMsg>(send_node, msg);
  }
}

TEST_F(TestSubphases, test_subphase_collective_resolution) {
  EXPECT_FALSE(thePhase()->isPendingSubphaseResolution());

  std::string label1{"my-test-collective-label1"};
  SubphaseType id1 = 0;

  vt_print(gen, "registering a collective subphase\n");

  // All nodes register the same rooted subphase label and should end up with
  // the same ID from the broker
  expected_id = id1 = thePhase()->registerCollectiveSubphase(label1);

  // Start the next phase, which would require all resolution is complete
  thePhase()->nextPhaseCollective();

  EXPECT_NE(id1, 0);

  auto const this_node = theContext()->getNode();
  if (this_node != 0) {
    NodeType const send_node = 0;
    auto msg = makeMessage<SubphaseIDTestMsg>(id1);
    theMsg()->sendMsg<SubphaseHandler, SubphaseIDTestMsg>(send_node, msg);
  }
}

TEST_F(TestSubphases, test_subphase_reduce_global_map) {
  EXPECT_FALSE(thePhase()->isPendingSubphaseResolution());

  std::string label1{"my-test-collective-label2"};
  std::string label2{"my-test-collective-label3"};
  std::string label3{"my-test-collective-label4"};

  vt_print(gen, "registering three collective subphases\n");

  // All nodes register the same rooted subphase label and should end up with
  // the same ID from the broker
  SubphaseType id1 = thePhase()->registerCollectiveSubphase(label1);
  SubphaseType id2 = thePhase()->registerCollectiveSubphase(label2);
  SubphaseType id3 = thePhase()->registerCollectiveSubphase(label3);

  std::string label4{"my-test-rooted-label2"};
  std::string label5{"my-test-rooted-label3"};
  std::string label6{"my-test-rooted-label4"};
  SubphaseType id4 = 0, id5 = 0, id6 = 0;

  vt_print(gen, "registering rooted subphases\n");

  // All nodes register the same rooted subphase label and should end up with
  // the same ID from the broker
  thePhase()->registerRootedSubphase(label4, [&id4](SubphaseType id) { id4=id; });
  thePhase()->registerRootedSubphase(label5, [&id5](SubphaseType id) { id5=id; });
  thePhase()->registerRootedSubphase(label6, [&id6](SubphaseType id) { id6=id; });

  // Start the next phase, which would require all label resolution to be
  // complete
  thePhase()->nextPhaseCollective();

  EXPECT_NE(id1, 0);
  EXPECT_NE(id2, 0);
  EXPECT_NE(id3, 0);
  EXPECT_NE(id4, 0);
  EXPECT_NE(id5, 0);
  EXPECT_NE(id6, 0);

  using IDMap = typename phase::subphase::SubphaseManager::IDMapType;

  IDMap out_map{};

  runInEpochCollective([&out_map]{
    thePhase()->reduceLabels([&](IDMap const& in){ out_map = in; });
  });

  if (theContext()->getNode() == 0) {
    EXPECT_EQ(out_map.size(), 6);
    EXPECT_EQ(out_map[label1], id1);
    EXPECT_EQ(out_map[label2], id2);
    EXPECT_EQ(out_map[label3], id3);
    EXPECT_EQ(out_map[label4], id4);
    EXPECT_EQ(out_map[label5], id5);
    EXPECT_EQ(out_map[label6], id6);
  }
}

}}} // end namespace vt::tests::unit
