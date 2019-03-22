/*
//@HEADER
// ************************************************************************
//
//                  test_termination_action_common.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#include <gtest/gtest.h>
#include <random>
#include <vector>

#include "test_parallel_harness.h"
#include "test_termination_channel_counting.h"

#if !defined INCLUDED_TERMINATION_ACTION_COMMON_H
#define INCLUDED_TERMINATION_ACTION_COMMON_H

#define DEBUG_TERM_ACTION 2

using namespace vt::tests::unit;
// set channel counting ranks
vt::NodeType channel::root = vt::uninitialized_destination;
vt::NodeType channel::me   = vt::uninitialized_destination;
vt::NodeType channel::all  = vt::uninitialized_destination;
std::unordered_map<vt::EpochType,channel::Data> channel::data;

/*
 * common actions and fixtures for:
 * - global termination
 * - rooted/collect epoch termination
 * - nested epochs termination.
 */
namespace vt { namespace tests { namespace unit { namespace action {

// ordering of 'addAction' with respect to 'finishedEpoch'
enum struct Order : int32_t {
  before = 0,
  after  = 1,
  misc   = 2
};

using Base = TestParallelHarnessParam<std::tuple<Order,bool,int>>;

struct BaseFixture : Base {

  virtual void SetUp(){
    // explicit inheritance
    Base::SetUp();
    // set channel counting ranks
    channel::root = 0;
    channel::me   = vt::theContext()->getNode();
    channel::all  = vt::theContext()->getNumNodes();
    vtAssert(channel::all > 1, "There should be at least two nodes");

    // retrieve test parameters
    auto const& values = GetParam();
    order_ = std::get<0>(values);
    useDS_ = std::get<1>(values);
    depth_ = std::get<2>(values);
  }

protected:
  Order order_ = Order::before;
  bool useDS_  = false;
  int depth_   = 1;
};

struct SimpleFixture : TestParallelHarness {
  virtual void SetUp() {
      // explicit inheritance
      TestParallelHarness::SetUp();
      // set channel counting ranks
      channel::root = 0;
      channel::me   = vt::theContext()->getNode();
      channel::all  = vt::theContext()->getNumNodes();
      vtAssertExpr(channel::all > 1);
  }
};

// epoch sequence creation
std::vector<vt::EpochType> newEpochSeq(
  int nb=1, bool rooted=false, bool useDS=false
);
// fictive distributed computation
void compute(vt::EpochType const& ep);
// check epoch termination
void verify(vt::EpochType const& ep, Order const& order);

/*
 * at the end of a given epoch:
 * - assign action to be processed
 * - trigger termination detection
 * - finish epoch
 */
void finalize(vt::EpochType const ep, Order const& order);

#if DEBUG_TERM_ACTION
  void print(std::string step, vt::EpochType const& ep, Order const& order);
#endif
}}}} // end namespace vt::tests::unit::action

#include "test_termination_action_common.impl.h"

#endif /*INCLUDED_TERMINATION_ACTION_COMMON_H*/
