/*
//@HEADER
// *****************************************************************************
//
//                            test_query_context.cc
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
#include "data_message.h"

#include "vt/vrt/collection/manager.h"

#include <cstdint>

namespace vt { namespace tests { namespace unit { namespace query {

struct TestQueryContext : TestParallelHarness { };

struct WorkMsg;

struct QueryTest : Collection<QueryTest,Index1D> {
  void work(WorkMsg* msg);
};

struct WorkMsg : CollectionMessage<QueryTest> {};

void QueryTest::work(WorkMsg* msg) {
  auto idx = vt::theCollection()->queryIndexContext<Index1D>();
  auto proxy = vt::theCollection()->queryProxyContext<Index1D>();
  EXPECT_EQ(*idx, this->getIndex());
  EXPECT_EQ(proxy, this->getProxy());
}

static constexpr int32_t const num_elms_per_node = 8;

TEST_F(TestQueryContext, test_query_context_broadcast_1) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();
  if (this_node == 0) {
    auto const& range = Index1D(num_nodes * num_elms_per_node);
    auto proxy = theCollection()->construct<QueryTest>(range);
    for (int i = 0; i < 10; i++) {
      proxy.broadcast<WorkMsg,&QueryTest::work>();
    }
  }
}

TEST_F(TestQueryContext, test_query_context_send_1) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();
  if (this_node == 0) {
    auto const& range = Index1D(num_nodes * num_elms_per_node);
    auto proxy = theCollection()->construct<QueryTest>(range);
    for (int i = 0; i < num_nodes * num_elms_per_node; i++) {
      proxy[i].send<WorkMsg,&QueryTest::work>();
    }
  }
}

}}}} // end namespace vt::tests::unit::query
