/*
//@HEADER
// *****************************************************************************
//
//                       test_termination_action_common.h
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

#include <gtest/gtest.h>
#include <random>
#include <vector>

#include "test_parallel_harness.h"
#include "test_termination_channel_counting.h"

#if !defined INCLUDED_TERMINATION_ACTION_COMMON_H
#define INCLUDED_TERMINATION_ACTION_COMMON_H

using namespace vt::tests::unit;

namespace vt { namespace tests { namespace unit { namespace channel {

// set channel counting ranks
extern vt::NodeType root;
extern vt::NodeType node;
extern vt::NodeType all;
extern std::unordered_map<vt::EpochType,channel::Data> data;
extern bool ok;

}}}} /* end namespace vt::tests::unit::channel */

/*
 * common actions and fixtures for:
 * - global termination
 * - rooted/collect epoch termination
 * - nested epochs termination.
 */
namespace vt { namespace tests { namespace unit { namespace action {
// shortcuts
using epoch_manip = ::vt::epoch::EpochManip;
using Base = TestParallelHarnessParam<std::tuple<int, bool, int>>;

struct BaseFixture : Base {
  void SetUp() override {
    // explicit inheritance
    Base::SetUp();
    // set channel counting ranks
    channel::root = 0;
    channel::node = vt::theContext()->getNode();
    channel::all  = vt::theContext()->getNumNodes();
    vtAssert(channel::all > 1, "There should be at least two nodes");

    // retrieve test parameters
    auto const& values = GetParam();
    order_ = std::get<0>(values);
    useDS_ = std::get<1>(values);
    depth_ = std::get<2>(values);

    vt_debug_print(
      term, node,
      "BaseFixture::setup: order_={}, useDS_={}, depth_={}\n",
      order_, useDS_, depth_
    );

  }

protected:
  int  order_ = 0;
  bool useDS_ = false;
  int  depth_ = 1;
};

struct SimpleFixture : TestParallelHarness {
  void SetUp() override {
    // explicit inheritance
    TestParallelHarness::SetUp();
    // set channel counting ranks
    channel::root = 0;
    channel::node = vt::theContext()->getNode();
    channel::all  = vt::theContext()->getNumNodes();
    vtAssert(channel::all > 1, "Should use at least two nodes");
  }
};

// epoch sequence creation
inline std::vector<vt::EpochType> generateEpochs(
  int nb = 1, bool rooted = false, bool useDS = false
);
// fictive distributed computation
inline void compute(vt::EpochType const& epoch);
// add the termination checker algorithm as a pending action
inline void add(vt::EpochType const& epoch, int order);
// finish the epoch
inline void finish(vt::EpochType const& epoch);
// set the flag indicating that the current
// epoch of the sequence is finished
inline void setOk(vt::Message* /*unused*/) { channel::ok = true; }


/*
 * at the end of a given epoch:
 * - assign action to be processed
 * - trigger termination detection
 * - finish epoch
 */
inline void finalize(vt::EpochType const& epoch, int order);

}}}} // end namespace vt::tests::unit::action

#include "test_termination_action_common.impl.h"

#endif /*INCLUDED_TERMINATION_ACTION_COMMON_H*/
