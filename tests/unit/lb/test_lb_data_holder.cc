/*
//@HEADER
// *****************************************************************************
//
//                           test_lb_data_holder.cc
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

#include "vt/elm/elm_id_bits.h"
#include <vt/collective/startup.h>
#include <vt/vrt/collection/balance/lb_data_holder.h>

#include "test_parallel_harness.h"

#include <nlohmann/json.hpp>

namespace vt { namespace tests { namespace unit { namespace lb {

using TestLBDataHolder = TestParallelHarness;

nlohmann::json create_entity_(std::string id_type, int id, int home, bool is_migratable) {
  nlohmann::json entity = {
    {id_type, id},
    {"type", "object"},
    {"collection_id", 7},
    {"index", {0}},
    {"home", home},
    {"migratable", is_migratable},
  };
  return entity;
}

nlohmann::json create_json_(
  std::string id_type, int id_1, int id_2, int home, int node,
  bool is_migratable) {

  auto entity_1 = create_entity_(id_type, id_1, home, is_migratable);
  auto entity_2 = create_entity_(id_type, id_2, home, is_migratable);

  // Generate JSON
  nlohmann::json j = {
    {"metadata", {{"rank", 0}, {"type", "LBDatafile"}}},
    {"phases",
      {{{"communications",
        {{{"bytes", 2.0},
          {"from", entity_1},
          {"messages", 1},
          {"to", entity_2},
          {"type", "SendRecv"}}}},
        {"id", 0},
        {"tasks",
        {
          {
            {"entity", entity_1},
            {"node", node},
            {"resource", "cpu"},
            {"time", 0.5},
          },
          {
            {"entity", entity_2},
            {"node", node},
            {"resource", "cpu"},
            {"time", 0.5},
          }
        }
      }}}
    }
  };

  return j;
}

void test_data_holder_elms(int seq_id_1, int home, int node, bool is_migratable) {
  // Create a second seq_id
  auto seq_id_2 = seq_id_1 + 1;

  // Determine encoded ID
  auto elm_1 =
    elm::ElmIDBits::createCollectionImpl(is_migratable, seq_id_1, home, node);
  auto encoded_id_1 = elm_1.id;

  // Create second encoded ID
  auto elm_2 =
    elm::ElmIDBits::createCollectionImpl(is_migratable, seq_id_2, home, node);
  auto encoded_id_2 = elm_2.id;

  // Create DataHolder and get resulting object elm
  auto simple_json_id = create_json_(
    "id", encoded_id_1, encoded_id_2, home, node, is_migratable);
  auto dh_id = vt::vrt::collection::balance::LBDataHolder(simple_json_id);

  // Create new DataHolder using "seq_id" and get elm
  auto simple_json_seq =
    create_json_("seq_id", seq_id_1, seq_id_2, home, node, is_migratable);
  auto dh_seq = vt::vrt::collection::balance::LBDataHolder(simple_json_seq);

  // Find both elms in each DataHolder
  auto it_id_1 = dh_id.node_data_[0].find(elm_1);
  auto it_seq_1 = dh_seq.node_data_[0].find(elm_1);
  auto it_id_2 = dh_id.node_data_[0].find(elm_2);
  auto it_seq_2 = dh_seq.node_data_[0].find(elm_2);

  // Assert that both elms exist
  ASSERT_NE(it_id_1, dh_id.node_data_[0].end());
  ASSERT_NE(it_seq_1, dh_seq.node_data_[0].end());
  ASSERT_NE(it_id_2, dh_id.node_data_[0].end());
  ASSERT_NE(it_seq_2, dh_seq.node_data_[0].end());

  // Compare the the elms in each DataHolder
  EXPECT_EQ(it_id_1->first, elm_1);
  EXPECT_EQ(it_seq_1->first, elm_1);
  EXPECT_EQ(it_id_2->first, elm_2);
  EXPECT_EQ(it_seq_2->first, elm_2);

  // Check the communication data
  auto comm_id = dh_id.node_comm_[0];
  auto comm_key_id = comm_id.begin()->first;
  auto comm_seq = dh_seq.node_comm_[0];
  auto comm_key_seq = comm_seq.begin()->first;

  // Ensure that we get the same CommKey from both id types
  EXPECT_EQ(comm_key_id, comm_key_seq);

  // Assert that both elms are present in the communication data
  EXPECT_EQ(comm_key_id.fromObj(), elm_1);
  EXPECT_EQ(comm_key_id.toObj(), elm_2);
  EXPECT_EQ(comm_key_seq.fromObj(), elm_1);
  EXPECT_EQ(comm_key_seq.toObj(), elm_2);
}

TEST_F(TestLBDataHolder, test_lb_data_holder_object_id) {
  // Run a variety of test cases (seq_id, home, node, is_migratable)
  test_data_holder_elms(0, 0, 0, false);
  test_data_holder_elms(0, 0, 0, true);
  test_data_holder_elms(0, 0, 2, false);
  test_data_holder_elms(0, 0, 1, true);
  test_data_holder_elms(1, 1, 0, false);
  test_data_holder_elms(2, 1, 1, true);
  test_data_holder_elms(3, 0, 1, false);
}

}}}} // end namespace vt::tests::unit::lb
