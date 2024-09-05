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

#include "test_harness.h"

#include <nlohmann/json.hpp>

namespace vt { namespace tests { namespace unit { namespace lb {

using TestLBDataHolder = TestHarness;

nlohmann::json create_basic_json(std::string id_type, int id,
                                 int home, int node, bool is_migratable) {
    nlohmann::json j = {
        {"metadata", {
            {"rank", 0},
            {"type", "LBDatafile"}
        }},
        {"phases", {
            {
                {"id", 0},
                {"tasks", {
                    {
                        {"entity", {
                            {"collection_id", 7},
                            {"index", {0}},
                            {"home", home},
                            {id_type, id},
                            {"migratable", is_migratable},
                            {"type", "object"}
                        }},
                        {"node", node},
                        {"resource", "cpu"},
                        {"time", 0.5},
                    }
                }}
            }
        }}
    };

    return j;
}

void test_data_holder_elms(int seq_id, int home, int node, bool is_migratable) {
    // Determine encoded ID
    auto elm = elm::ElmIDBits::createCollectionImpl(is_migratable, seq_id, home, node);
    auto encoded_id = elm.id;

    // Create DataHolder and get resulting object elm
    auto simple_json_id = create_basic_json("id", encoded_id, home, node, is_migratable);
    auto dh_id = vt::vrt::collection::balance::LBDataHolder(simple_json_id);
    auto elm_id = dh_id.node_data_[0].begin()->first;

    // Create new DataHolder using "seq_id" and get elm
    auto simple_json_seq = create_basic_json("seq_id", seq_id, home, node, is_migratable);
    auto dh_seq = vt::vrt::collection::balance::LBDataHolder(simple_json_seq);
    auto elm_seq = dh_seq.node_data_[0].begin()->first;

    // Assert that both elms are equal (have the same id)
    EXPECT_EQ(elm_id, elm_seq);
    EXPECT_EQ(elm_id, elm);
}

TEST_F(TestLBDataHolder, test_lb_data_holder_no_comms_object_id) {
    // Run a variety of test cases (seq_id, home, node, is_migratable)
    test_data_holder_elms(0,0,0,false);
    test_data_holder_elms(0,0,0,true);
    test_data_holder_elms(0,0,2,false);
    test_data_holder_elms(0,0,1,true);
    test_data_holder_elms(1,1,0,false);
    test_data_holder_elms(2,1,9,true);
    test_data_holder_elms(3,0,1,false);
}

}}}} // end namespace vt::tests::unit::lb
