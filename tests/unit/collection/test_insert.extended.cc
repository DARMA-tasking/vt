/*
//@HEADER
// *****************************************************************************
//
//                           test_insert.extended.cc
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

#include "vt/transport.h"

#include <cstdint>

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::collective;
using namespace vt::tests::unit;

struct WorkMsg;

static int32_t num_inserted = 0;
static int32_t num_work = 0;

struct InsertTest : InsertableCollection<InsertTest,Index1D> {
  InsertTest() : InsertableCollection<InsertTest,Index1D>() {
    num_inserted++;
    // ::fmt::print(
    //   "{}: inserting on node {}\n", idx.x(), theContext()->getNode()
    // );
  }

  void work(WorkMsg* msg);
};

void InsertTest::work(WorkMsg* msg) {
  //::fmt::print("node={}: num_work={}\n", theContext()->getNode(), num_work);
  num_work++;
}

struct WorkMsg : CollectionMessage<InsertTest> {};
using ColProxyType = CollectionIndexProxy<InsertTest,Index1D>;

struct TestInsert : TestParallelHarness { };

static constexpr int32_t const num_elms_per_node = 8;

TEST_F(TestInsert, test_insert_dense_1) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  vt::runInEpochCollective([&]{
    if (this_node == 0) {
      auto const& range = Index1D(num_nodes * num_elms_per_node);
      auto proxy = theCollection()->construct<InsertTest>(range);
      for (auto i = 0; i < range.x(); i++) {
        proxy[i].insert();
      }
    }
  });
  EXPECT_EQ(num_inserted, num_elms_per_node);
  num_inserted = 0;
}

TEST_F(TestInsert, test_insert_sparse_1) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  vt::runInEpochCollective([&]{
    if (this_node == 0) {
      auto const& range = Index1D(num_nodes * num_elms_per_node * 16);
      auto proxy = theCollection()->construct<InsertTest>(range);
      for (auto i = 0; i < range.x(); i+=16) {
        proxy[i].insert();
      }
    }
  });
  /// ::fmt::print("num inserted={}\n", num_inserted);
  // Relies on default mapping equally distributing
  EXPECT_EQ(num_inserted, num_elms_per_node);
  num_inserted = 0;
}

TEST_F(TestInsert, test_insert_dense_node_1) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  vt::runInEpochCollective([&]{
    if (this_node == 0) {
      auto const& range = Index1D(num_nodes * num_elms_per_node);
      auto proxy = theCollection()->construct<InsertTest>(range);
      for (auto i = 0; i < range.x(); i++) {
        proxy[i].insert(this_node);
      }
    }
  });
  /// ::fmt::print("num inserted={}\n", num_inserted);
  // Relies on default mapping equally distributing
  if (this_node == 0) {
    EXPECT_EQ(num_inserted, num_elms_per_node * num_nodes);
  }
  num_inserted = 0;
}

TEST_F(TestInsert, test_insert_sparse_node_1) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  vt::runInEpochCollective([&]{
    if (this_node == 0) {
      auto const& range = Index1D(num_nodes * num_elms_per_node * 16);
      auto proxy = theCollection()->construct<InsertTest>(range);
      for (auto i = 0; i < range.x(); i+=16) {
        proxy[i].insert(this_node);
      }
    }
  });
    /// ::fmt::print("num inserted={}\n", num_inserted);
    // Relies on default mapping equally distributing
  if (this_node == 0) {
    EXPECT_EQ(num_inserted, num_elms_per_node * num_nodes);
  }
  num_inserted = 0;
}

TEST_F(TestInsert, test_insert_send_dense_node_1) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  vt::runInEpochCollective([&]{
    if (this_node == 0) {
      auto const& range = Index1D(num_nodes * num_elms_per_node);
      auto proxy = theCollection()->construct<InsertTest>(range);
      for (auto i = 0; i < range.x(); i++) {
        proxy[i].insert((this_node + 1) % num_nodes);
        auto msg = makeMessage<WorkMsg>();
        proxy[i].send<WorkMsg,&InsertTest::work>(msg.get());
        // ::fmt::print("sending to {}\n", i);
      }
    }
  });
  /// ::fmt::print("num inserted={}\n", num_inserted);
  // Relies on default mapping equally distributing
  if (this_node == 1 || (this_node == 0 && num_nodes == 1)) {
    EXPECT_EQ(num_inserted, num_elms_per_node * num_nodes);
    EXPECT_EQ(num_work, num_elms_per_node * num_nodes);
  }
  num_inserted = 0;
  num_work = 0;
}

TEST_F(TestInsert, test_insert_send_sparse_node_1) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  vt::runInEpochCollective([&]{
    if (this_node == 0) {
      auto const& range = Index1D(num_nodes * num_elms_per_node * 16);
      auto proxy = theCollection()->construct<InsertTest>(range);
      for (auto i = 0; i < range.x(); i+=16) {
        proxy[i].insert((this_node + 1) % num_nodes);
        auto msg = makeMessage<WorkMsg>();
        proxy[i].send<WorkMsg,&InsertTest::work>(msg.get());
      }
    }
  });
  /// ::fmt::print("num inserted={}\n", num_inserted);
  // Relies on default mapping equally distributing
  if (this_node == 1 || (this_node == 0 && num_nodes == 1)) {
    EXPECT_EQ(num_inserted, num_elms_per_node * num_nodes);
    EXPECT_EQ(num_work, num_elms_per_node * num_nodes);
  }
  num_inserted = 0;
  num_work = 0;
}

}}} // end namespace vt::tests::unit
