/*
//@HEADER
// *****************************************************************************
//
//                           test_list_insert_here.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#include <gtest/gtest.h>

#include "test_parallel_harness.h"

#include <vector>
#include <tuple>

namespace vt::tests::unit::list_insert_here {

using TestListInsertHere = TestParallelHarness;

void reduceTarget(int count_elems) {
  auto const nnodes = theContext()->getNumNodes();
  fmt::print("count_elems={}, {}\n", count_elems, nnodes*(nnodes+1)/2);
  EXPECT_EQ(count_elems, nnodes*(nnodes+1)/2);
}

struct MyCol : vt::Collection<MyCol, vt::Index2D> {

  void handler() {
    count++;
    auto proxy = getCollectionProxy();
    proxy.reduce<reduceTarget, collective::PlusOp>(/*target*/ 0, 1);
  }

  virtual ~MyCol() {
    EXPECT_EQ(count, 1);
  }

  int count = 0;
};

vt::NodeType collectionMap(vt::Index2D* idx, vt::Index2D*, vt::NodeType) {
  return idx->x();
}

TEST_F(TestListInsertHere, test_list_insert_here1) {

  std::vector<std::tuple<vt::Index2D, std::unique_ptr<MyCol>>> elms;

  auto const this_node = theContext()->getNode();

  for (NodeType i = 0; i < this_node+1; i++) {
    elms.emplace_back(vt::Index2D{(int)this_node, (int)i}, std::make_unique<MyCol>());
  }

  auto proxy = makeCollection<MyCol>("test list insert here")
    .listInsertHere(std::move(elms))
    .template mapperFunc<collectionMap>()
    .wait();

  runInEpochCollective([&]{
    // rooted broadcast to test spanning tree
    if (this_node == 0) {
      proxy.broadcast<&MyCol::handler>();
    }
  });
}

TEST_F(TestListInsertHere, test_list_insert_here_sparse2) {

  std::vector<std::tuple<vt::Index2D, std::unique_ptr<MyCol>>> elms;

  auto const this_node = theContext()->getNode();

  // node 0 is empty, node 1 makes up for it so the math works out
  auto end = this_node == 0 ? 0 : (this_node == 1 ? this_node+2 : this_node+1);
  for (NodeType i = 0; i < end; i++) {
    elms.emplace_back(vt::Index2D{(int)this_node, (int)i}, std::make_unique<MyCol>());
  }

  auto proxy = makeCollection<MyCol>("test list insert here")
    .listInsertHere(std::move(elms))
    .template mapperFunc<collectionMap>()
    .wait();

  runInEpochCollective([&]{
    // rooted broadcast to test spanning tree
    if (this_node == 0) {
      proxy.broadcast<&MyCol::handler>();
    }
  });
}

} // end namespace vt::tests::unit::list_insert_here
