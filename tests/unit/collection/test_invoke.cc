/*
//@HEADER
// *****************************************************************************
//
//                               test_invoke.cc
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

#include "test_parallel_harness.h"
#include "test_collection_common.h"

#include "vt/vrt/collection/manager.h"

#include <gtest/gtest.h>
#include <numeric>

namespace vt { namespace tests { namespace unit { namespace invoke {

static bool handler_invoked = false;

template <typename TestCol>
struct TestMsg : CollectionMessage<TestCol> {
  using IdxType = typename TestCol::IndexType;

  explicit TestMsg(IdxType index_value) {
    index_value_ = index_value;
  }

  IdxType index_value_ = IdxType{-1};
};

struct TestCol : public Collection<TestCol, Index1D> {
  void memberHandler(TestMsg<TestCol>* msg) {
    handler_invoked = true;
    EXPECT_EQ(getIndex(), msg->index_value_);
  }

  int accumulateVec(IndexType idx, const std::vector<int32_t>& vec) {
    handler_invoked = true;
    EXPECT_EQ(getIndex(), idx);

    return std::accumulate(std::begin(vec), std::end(vec), 0);
  }
};

struct TestCollectionInvoke : TestParallelHarness {};

TEST_F(TestCollectionInvoke, test_collection_invoke_1) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();
  auto const num_elems = Index1D{static_cast<int>(num_nodes)};

  auto proxy = theCollection()->constructCollective<TestCol>(num_elems);

  auto const dest_elem = Index1D{static_cast<int>(this_node)};

  // Message handler
  {
    proxy[dest_elem].invoke<TestMsg<TestCol>, &TestCol::memberHandler>(
      dest_elem);

    EXPECT_EQ(handler_invoked, true);

    handler_invoked = false;
  }

  // Non-message function
  {
    auto const accumulate_result =
      proxy[dest_elem]
        .invoke<decltype(&TestCol::accumulateVec), &TestCol::accumulateVec>(
          dest_elem, std::vector<int32_t>{2, 4, 5}
        );

    EXPECT_EQ(accumulate_result, 11);
    EXPECT_EQ(handler_invoked, true);
  }
}

}}}} // end namespace vt::tests::unit::invoke
