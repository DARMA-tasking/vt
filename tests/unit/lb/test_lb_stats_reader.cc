/*
//@HEADER
// *****************************************************************************
//
//                           test_lb_stats_reader.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2020 National Technology & Engineering Solutions of Sandia, LLC
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

#include <fstream>
#include <string>
#include <vector>

#include <vt/vrt/collection/balance/node_stats.h>
#include <vt/vrt/collection/balance/stats_data.h>
#include <vt/vrt/collection/balance/stats_restart_reader.h>
#include <vt/utils/json/json_appender.h>

#include "test_parallel_harness.h"
#include "data_message.h"

namespace vt { namespace tests { namespace unit {

struct TestLBStatsReader : TestParallelHarness { };
using ElementIDType = uint64_t;

ElementIDType getElemPermID(size_t elm, NodeType inode) {
  //--- Formula inside the function "Stats::getNextElem()"
  return static_cast<ElementIDType>((elm << 32) | inode);
}

TEST_F(TestLBStatsReader, test_lb_stats_read_1) {

  // Iter 0
  // Node 0 -> [0, 1, 2, 3, 4] // local ID
  // Node 1 -> [0, 1, 2, 3, 4] // local ID
  // Node 2 -> [0, 1, 2, 3, 4] // local ID

  // Iter 1  (send most elements to the last node)
  // Node 0 -> [0] // local ID
  // Node 1 -> [0] // local ID
  // Node 2 -> [0, 1, 2, 3, 4, {1,0}, {2, 0}, ...]

  // Iter 2  (send the elements back to their initial node)
  // Node 0 -> [0, 1, 2, 3, 4] // local ID
  // Node 1 -> [0, 1, 2, 3, 4] // local ID
  // Node 2 -> [0, 1, 2, 3, 4] // local ID

  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  using vt::vrt::collection::balance::ElementIDStruct;

  const size_t numElements = 5;
  std::vector<ElementIDStruct> myElemList(numElements);

  for (size_t ii = 0; ii < numElements; ++ii) {
	//--- Shift by 1 to avoid the null permID
    myElemList[ii] = ElementIDStruct{
      getElemPermID(ii+1, this_node), this_node, this_node
    };
  }

  using JSONAppender = vt::util::json::Appender<std::stringstream>;

  std::stringstream stream{std::ios_base::out | std::ios_base::in};
  auto ap = std::make_unique<JSONAppender>("phases", std::move(stream), true);

  using vt::vrt::collection::balance::StatsData;
  auto sd = std::make_unique<StatsData>();

  PhaseType phase = 0;
  double tval = 0.0;

  //--- Iteration 0
  for (auto&& elmID : myElemList) {
    //--- Use a dummy time value as it is not used.
    sd->node_data_[phase][elmID] = tval;
  }

  //--- Iteration 1
  phase += 1;
  if (this_node != num_nodes - 1) {
    sd->node_data_[phase][myElemList[0]] = tval;
  } else {
    for (auto&& elmID : myElemList) {
      sd->node_data_[phase][elmID] = tval;
    }
    for (NodeType in = 0; in+1 < num_nodes; ++in) {
      for (uint64_t elmID = 1; elmID < numElements; ++elmID) {
        auto permID = ElementIDStruct{getElemPermID(elmID+1, in), this_node, in};
        sd->node_data_[phase][permID] = tval;
      }
    }
  }

  //--- Iteration 2
  phase += 1;
  for (auto&& elmID : myElemList) {
    sd->node_data_[phase][elmID] = tval;
  }

  phase += 1;

  // Write the stats
  for (PhaseType p = 0; p < phase; p++) {
    auto json = sd->toJson(p);
    ap->addElm(*json);
  }

  stream = ap->finish();

  //--- Start the testing
  auto ptr = vrt::collection::balance::StatsRestartReader::construct();
  ptr->readStatsFromStream(std::move(stream));

  //--- Spin here so the test does not end before the communications complete
  vt::theSched()->runSchedulerWhile([]{ return !rt->isTerminated(); });

  //--- Check the read values

  auto const &migrationList = ptr->getMigrationList();
  auto const numIters = migrationList.size();

  EXPECT_TRUE(numIters == phase - 1);

  //--- Iteration 0 -> 1
  size_t phaseID = 0;
  auto myList = migrationList[phaseID];
  EXPECT_TRUE(myList.size() % 2 == 0);
  if (myList.size() > 0) {
    for (size_t ii = 1; ii < numElements; ++ii) {
      auto myPermID = myElemList[ii];
      auto it = std::find(myList.begin(), myList.end(), myPermID.id);
      EXPECT_TRUE(it != myList.end());
      size_t shift = static_cast<size_t>(it - myList.begin());
      EXPECT_TRUE(myList[shift+1] == static_cast<ElementIDType>(num_nodes - 1));
    }
  }

  //--- Iteration 1 -> 2
  phaseID = 1;
  myList = migrationList[phaseID];
  if (this_node != num_nodes - 1) {
    EXPECT_TRUE(myList.size() == 0);
  }
  else {
    EXPECT_TRUE(myList.size() % 2 == 0);
    for (NodeType in = 0; in+1 < num_nodes; ++in) {
      for (ElementIDType elmID = 1; elmID < numElements; ++elmID) {
        ElementIDType permID = getElemPermID(elmID + 1, in);
        auto it = std::find(myList.begin(), myList.end(), permID);
        EXPECT_TRUE(it != myList.end());
        size_t shift = static_cast<size_t>(it - myList.begin());
        EXPECT_TRUE(myList[shift+1] == static_cast<ElementIDType>(in));
      }
    }
  }

}

}}} // end namespace vt::tests::unit
