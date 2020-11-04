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

#include <gtest/gtest.h>

#include "test_parallel_harness.h"
#include "test_collection_common.h"

#include "vt/transport.h"

namespace vt { namespace tests { namespace unit { namespace invoke {

static bool handlerInvoked = false;

template <typename TestCol>
struct TestMsg : CollectionMessage<TestCol> {
  using IdxType = typename TestCol::IndexType;

  explicit TestMsg(IdxType indexVal) {
    indexValue_ = indexVal;
  }

  IdxType indexValue_ = IdxType{-1};
};

struct TestCol : public Collection<TestCol, Index1D> {
  void MemberHandler(TestMsg<TestCol>* msg) {
    handlerInvoked = true;
    EXPECT_EQ(getIndex(), msg->indexValue_);
  }
};

struct TestCollectionInvoke : TestParallelHarness {};

TEST_F(TestCollectionInvoke, test_collection_invoke_1) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();
  auto const num_elems = Index1D{4};

  auto proxy = theCollection()->constructCollective<TestCol>(num_elems);

  auto const destElem = Index1D{this_node + num_elems.x() / num_nodes - 1};
  proxy[destElem].invoke<TestMsg<TestCol>, &TestCol::MemberHandler>(destElem);

  EXPECT_EQ(handlerInvoked, true);
}

}}}} // end namespace vt::tests::unit::invoke
