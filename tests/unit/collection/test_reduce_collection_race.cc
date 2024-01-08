/*
//@HEADER
// *****************************************************************************
//
//                        test_reduce_collection_race.cc
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

#include <vt/transport.h>
#include "test_parallel_harness.h"

namespace vt { namespace tests { namespace unit { namespace race {

using TestReduceCollectionRace = TestParallelHarnessParam<int>;

struct MyCol : vt::Collection<MyCol, vt::Index1D> {};

static int multiplier = 0;

static void reduceTarget(MyCol* col, int val) {
  auto const num_nodes = theContext()->getNumNodes();
  auto const num_elems = num_nodes * multiplier;
  fmt::print("reduce finished: val={}, num_elems={}\n", val, num_elems);
  EXPECT_EQ(val, (num_elems * (num_elems-1))/2);
}

static void handler(MyCol* col) {
  auto proxy = col->getCollectionProxy();
  int val = col->getIndex().x();
  proxy.reduce<reduceTarget, vt::collective::PlusOp>(proxy[0], val);
}

TEST_P(TestReduceCollectionRace, test_reduce_race_1) {
  auto const num_nodes = theContext()->getNumNodes();

  multiplier = GetParam();
  auto const range = Index1D(multiplier * num_nodes);
  auto proxy = theCollection()->constructCollective<MyCol>(
    range, "test_reduce_race_1"
  );

  proxy.broadcastCollective<&handler>();
  proxy.broadcastCollective<&handler>();
  proxy.broadcastCollective<&handler>();
  proxy.broadcastCollective<&handler>();
  proxy.broadcastCollective<&handler>();
}

INSTANTIATE_TEST_SUITE_P(
  InstantiationName, TestReduceCollectionRace, ::testing::Range(1, 5)
);

}}}} // end namespace vt::tests::unit::race
