/*
//@HEADER
// *****************************************************************************
//
//                           test_insert.extended.cc
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

#include <gtest/gtest.h>

#include "test_parallel_harness.h"
#include "test_collection_common.h"
#include "data_message.h"

#include "vt/vrt/collection/manager.h"

#include <cstdint>

namespace vt { namespace tests { namespace unit { namespace insert {

using namespace vt;
using namespace vt::collective;
using namespace vt::tests::unit;

struct WorkMsg;

static int32_t num_inserted = 0;
static int32_t num_deleted = 0;
static int32_t num_work = 0;

struct InsertTest : Collection<InsertTest,Index1D> {
  InsertTest() {
    num_inserted++;
    ::fmt::print(
      "{}: inserting on node {}\n", getIndex(), theContext()->getNode()
    );
  }

  virtual ~InsertTest() {
    num_deleted++;
  }

  void work(WorkMsg* msg);
};

void InsertTest::work(WorkMsg* msg) {
  ::fmt::print("node={}: num_work={}, idx={}\n", theContext()->getNode(), num_work, getIndex());
  num_work++;
}

struct WorkMsg : CollectionMessage<InsertTest> {};
using ColProxyType = CollectionIndexProxy<InsertTest,Index1D>;

struct TestInsert : TestParallelHarness { };

static constexpr int32_t const num_elms_per_node = 8;

TEST_F(TestInsert, test_insert_dense_1) {
  auto const this_node = theContext()->getNode();
  auto const num_nodes = theContext()->getNumNodes();

  auto const range = Index1D(num_nodes * num_elms_per_node);
  auto proxy = vt::makeCollection<InsertTest>()
    .collective(true)
    .dynamicMembership(true)
    .bounds(range)
    .wait();

  {
    auto token = proxy.beginModification();
    if (this_node == 0) {
      for (auto i = 0; i < range.x(); i++) {
        proxy[i].insert(token);
      }
    }
    proxy.finishModification(std::move(token));
  }

  EXPECT_EQ(num_inserted, num_elms_per_node);
  num_inserted = 0;

  {
    auto token = proxy.beginModification();
    if (this_node == 0) {
      for (auto i = 0; i < range.x(); i++/*=2*/) {
        proxy[i].destroy(token);
      }
    }
    proxy.finishModification(std::move(token));
  }

  EXPECT_EQ(num_deleted, num_elms_per_node/*/2*/);
  num_deleted = 0;

  // Everyone broadcast to test null spanning tree
  proxy.broadcast<WorkMsg, &InsertTest::work>();
}

TEST_F(TestInsert, test_insert_sparse_1) {
  auto const this_node = theContext()->getNode();
  auto const num_nodes = theContext()->getNumNodes();

  auto const range = Index1D(num_nodes * num_elms_per_node * 16);
  auto proxy = vt::makeCollection<InsertTest>()
    .collective(true)
    .dynamicMembership(true)
    .bounds(range)
    .wait();

  auto token = proxy.beginModification();
  if (this_node == 0) {
    for (auto i = 0; i < range.x(); i+=16) {
      proxy[i].insert(token);
    }
  }
  proxy.finishModification(std::move(token));

  /// ::fmt::print("num inserted={}\n", num_inserted);
  // Relies on default mapping equally distributing
  EXPECT_EQ(num_inserted, num_elms_per_node);
  num_inserted = 0;
}

TEST_F(TestInsert, test_insert_dense_node_1) {
  auto const this_node = theContext()->getNode();
  auto const num_nodes = theContext()->getNumNodes();

  auto const range = Index1D(num_nodes * num_elms_per_node);
  auto proxy = vt::makeCollection<InsertTest>()
    .collective(true)
    .dynamicMembership(true)
    .bounds(range)
    .wait();

  auto token = proxy.beginModification();
  if (this_node == 0) {
    for (auto i = 0; i < range.x(); i++) {
      proxy[i].insertAt(token, this_node);
    }
  }
  proxy.finishModification(std::move(token));

  /// ::fmt::print("num inserted={}\n", num_inserted);
  // Relies on default mapping equally distributing
  if (this_node == 0) {
    EXPECT_EQ(num_inserted, num_elms_per_node * num_nodes);
  }
  num_inserted = 0;
}

TEST_F(TestInsert, test_insert_sparse_node_1) {
  auto const this_node = theContext()->getNode();
  auto const num_nodes = theContext()->getNumNodes();

  auto const range = Index1D(num_nodes * num_elms_per_node * 16);
  auto proxy = vt::makeCollection<InsertTest>()
    .collective(true)
    .dynamicMembership(true)
    .bounds(range)
    .wait();

  auto token = proxy.beginModification();
  if (this_node == 0) {
    for (auto i = 0; i < range.x(); i+=16) {
      proxy[i].insertAt(token, this_node);
    }
  }
  proxy.finishModification(std::move(token));

  /// ::fmt::print("num inserted={}\n", num_inserted);
  // Relies on default mapping equally distributing
  if (this_node == 0) {
    EXPECT_EQ(num_inserted, num_elms_per_node * num_nodes);
  }
  num_inserted = 0;
}

TEST_F(TestInsert, test_insert_send_dense_node_1) {
  auto const this_node = theContext()->getNode();
  auto const num_nodes = theContext()->getNumNodes();

  auto const range = Index1D(num_nodes * num_elms_per_node);
  auto proxy = vt::makeCollection<InsertTest>()
    .collective(true)
    .dynamicMembership(true)
    .bounds(range)
    .wait();

  auto token = proxy.beginModification();
  if (this_node == 0) {
    for (auto i = 0; i < range.x(); i++) {
      proxy[i].insertAt(token, (this_node + 1) % num_nodes);
      // ::fmt::print("sending to {}\n", i);
    }
  }
  proxy.finishModification(std::move(token));

  runInEpochCollective([&]{
    if (this_node == 0) {
      for (auto i = 0; i < range.x(); i++) {
        proxy[i].send<WorkMsg,&InsertTest::work>();
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
  auto const this_node = theContext()->getNode();
  auto const num_nodes = theContext()->getNumNodes();

  auto const range = Index1D(num_nodes * num_elms_per_node * 16);
  auto proxy = vt::makeCollection<InsertTest>()
    .collective(true)
    .dynamicMembership(true)
    .bounds(range)
    .wait();

  auto token = proxy.beginModification();
  if (this_node == 0) {
    for (auto i = 0; i < range.x(); i+=16) {
      proxy[i].insertAt(token, (this_node + 1) % num_nodes);
    }
  }
  proxy.finishModification(std::move(token));

  runInEpochCollective([&]{
    if (this_node == 0) {
      for (auto i = 0; i < range.x(); i+=16) {
        proxy[i].send<WorkMsg,&InsertTest::work>();
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

}}}} // end namespace vt::tests::unit::insert
