/*
//@HEADER
// *****************************************************************************
//
//                          test_temperedlb.nompi.cc
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

#include <vt/vrt/collection/balance/lb_common.h>
#include <vt/vrt/collection/balance/baselb/baselb.h>
#include <vt/vrt/collection/balance/temperedlb/temperedlb.h>

#include "test_harness.h"

namespace vt { namespace tests { namespace unit {

using TestTemperedLB = TestHarness;
using ElementIDStruct = vt::vrt::collection::balance::ElementIDStruct;
using ElementIDType = vt::vrt::collection::balance::ElementIDType;
using ObjectOrdering = vt::vrt::collection::lb::ObjectOrderEnum;

TimeType setupProblem(
  std::unordered_map<ElementIDStruct, TimeType> &cur_objs
) {
  // total load of 29 seconds
  cur_objs.emplace(ElementIDStruct{3,0,0}, 4.0);
  cur_objs.emplace(ElementIDStruct{5,0,0}, 5.0);
  cur_objs.emplace(ElementIDStruct{2,0,0}, 9.0);
  cur_objs.emplace(ElementIDStruct{0,0,0}, 2.0);
  cur_objs.emplace(ElementIDStruct{1,0,0}, 6.0);
  cur_objs.emplace(ElementIDStruct{4,0,0}, 3.0);

  // compute the load for this processor
  TimeType my_load = 0;
  for (auto &obj : cur_objs) {
    my_load += obj.second;
  }
  return my_load;
}

void orderAndVerify(
  ObjectOrdering order,
  const std::unordered_map<ElementIDStruct, TimeType> &cur_objs,
  TimeType my_load_ms, TimeType target_load_ms,
  const std::vector<ElementIDType> &soln
) {
  // have TemperedLB order the objects
  auto ordered_objs = vt::vrt::collection::lb::TemperedLB::orderObjects(
    order, cur_objs, my_load_ms, target_load_ms
  );

  // verify correctness of the returned ordering
  int i = 0;
  for (auto obj_id : ordered_objs) {
    EXPECT_EQ(obj_id.id, soln[i++]);
  }
}

void orderUsingOverloadAndVerify(
  ObjectOrdering order, TimeType over_avg_sec /* my_load-target_load */,
  const std::vector<ElementIDType> &soln
) {
  std::unordered_map<ElementIDStruct, TimeType> cur_objs;
  TimeType my_load_ms = vt::vrt::collection::lb::BaseLB::loadMilli(
    setupProblem(cur_objs)
  );

  // we know how overloaded we are and need to compute the target load
  TimeType target_load_ms = my_load_ms -
    vt::vrt::collection::lb::BaseLB::loadMilli(over_avg_sec);

  orderAndVerify(order, cur_objs, my_load_ms, target_load_ms, soln);
}

void orderUsingTargetLoadAndVerify(
  ObjectOrdering order, TimeType target_load_sec,
  const std::vector<ElementIDType> &soln
) {
  std::unordered_map<ElementIDStruct, TimeType> cur_objs;
  TimeType my_load_ms = vt::vrt::collection::lb::BaseLB::loadMilli(
    setupProblem(cur_objs)
  );

  // we were given the target load directly but need to convert units
  TimeType target_load_ms = vt::vrt::collection::lb::BaseLB::loadMilli(
    target_load_sec
  );

  orderAndVerify(order, cur_objs, my_load_ms, target_load_ms, soln);
}

///////////////////////////////////////////////////////////////////////////

TEST_F(TestTemperedLB, test_temperedlb_ordering_elmid) {
  ObjectOrdering order = ObjectOrdering::ElmID;
  TimeType over_avg = 4.5;
  // result will be independent of over_avg
  std::vector<ElementIDType> soln = {0, 1, 2, 3, 4, 5};

  orderUsingOverloadAndVerify(order, over_avg, soln);
}

///////////////////////////////////////////////////////////////////////////

TEST_F(TestTemperedLB, test_temperedlb_ordering_fewestmigrations_intermediate) {
  ObjectOrdering order = ObjectOrdering::FewestMigrations;
  TimeType over_avg = 4.5;
  // single_obj_load will be 5.0
  // load order will be 5.0, 4.0, 3.0, 2.0, 6.0, 9.0
  std::vector<ElementIDType> soln = {5, 3, 4, 0, 1, 2};

  orderUsingOverloadAndVerify(order, over_avg, soln);
}

TEST_F(TestTemperedLB, test_temperedlb_ordering_fewestmigrations_largest) {
  ObjectOrdering order = ObjectOrdering::FewestMigrations;
  TimeType over_avg = 12.5;
  // single_obj_load will be 9.0
  // load order will be 9.0, 6.0, 5.0, 4.0, 3.0, 2.0
  std::vector<ElementIDType> soln = {2, 1, 5, 3, 4, 0};

  orderUsingOverloadAndVerify(order, over_avg, soln);
}

TEST_F(TestTemperedLB, test_temperedlb_ordering_fewestmigrations_smallest) {
  ObjectOrdering order = ObjectOrdering::FewestMigrations;
  TimeType over_avg = 1.5;
  // single_obj_load will be 2.0
  // load order will be 2.0, 3.0, 4.0, 5.0, 6.0, 9.0
  std::vector<ElementIDType> soln = {0, 4, 3, 5, 1, 2};

  orderUsingOverloadAndVerify(order, over_avg, soln);
}

///////////////////////////////////////////////////////////////////////////

TEST_F(TestTemperedLB, test_temperedlb_ordering_smallobjects_intermediate) {
  ObjectOrdering order = ObjectOrdering::SmallObjects;
  TimeType over_avg = 4.5;
  // marginal_obj_load will be 3.0
  // load order will be 3.0, 2.0, 4.0, 5.0, 6.0, 9.0
  std::vector<ElementIDType> soln = {4, 0, 3, 5, 1, 2};

  orderUsingOverloadAndVerify(order, over_avg, soln);
}

TEST_F(TestTemperedLB, test_temperedlb_ordering_smallobjects_largest) {
  ObjectOrdering order = ObjectOrdering::SmallObjects;
  TimeType target_load = 0.5;
  // marginal_obj_load will be 9.0
  // load order will be 9.0, 6.0, 5.0, 4.0, 3.0, 2.0
  std::vector<ElementIDType> soln = {2, 1, 5, 3, 4, 0};

  orderUsingTargetLoadAndVerify(order, target_load, soln);
}

TEST_F(TestTemperedLB, test_temperedlb_ordering_smallobjects_smallest) {
  ObjectOrdering order = ObjectOrdering::SmallObjects;
  TimeType over_avg = 1.5;
  // marginal_obj_load will be 2.0
  // load order will be 2.0, 3.0, 4.0, 5.0, 6.0, 9.0
  std::vector<ElementIDType> soln = {0, 4, 3, 5, 1, 2};

  orderUsingOverloadAndVerify(order, over_avg, soln);
}

///////////////////////////////////////////////////////////////////////////

TEST_F(TestTemperedLB, test_temperedlb_ordering_largestobjects) {
  ObjectOrdering order = ObjectOrdering::LargestObjects;
  TimeType over_avg = 4.5;
  // result will be independent of over_avg
  std::vector<ElementIDType> soln = {2, 1, 5, 3, 4, 0};

  orderUsingOverloadAndVerify(order, over_avg, soln);
}

}}} // end namespace vt::tests::unit
