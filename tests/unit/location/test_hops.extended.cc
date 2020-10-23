/*
//@HEADER
// *****************************************************************************
//
//                            test_hops.extended.cc
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

#include "vt/transport.h"

namespace vt { namespace tests { namespace unit {

static int num_elms = 16;
static int width = 1;

using TestHops = TestParallelHarness;


struct TestColl : Collection<TestColl,vt::Index2D> {
  TestColl() = default;

  struct TestMsg : CollectionMessage<TestColl> {
    TestMsg() = default;
    explicit TestMsg(bool in_do_check)
      : do_check_(in_do_check)
    { }
    bool do_check_ = false;
  };

  struct NeighborMsg : vt::CollectionMessage<TestColl> {
    NeighborMsg() = default;
    explicit NeighborMsg(vt::Index2D in_idx, bool in_do_check)
      : idx_(in_idx),
        do_check_(in_do_check)
    { }
    vt::Index2D idx_;
    bool do_check_ = false;
  };

  int wrap(int dim, int maxd) {
    while (dim < 0) {
      dim += maxd;
    }
    if (dim >= maxd) {
      return dim % maxd;
    }
    return dim;
  }

  void doWork(TestMsg* msg) {
    auto proxy = this->getCollectionProxy();
    auto idx = this->getIndex();
    auto x = idx.x();
    auto y = idx.y();
    auto num_nodes = vt::theContext()->getNumNodes();

    for (int x1 = x - width; x1 < x + width; x1++) {
      for (int y1 = y - width; y1 < y + width; y1++) {
        proxy(wrap(x1,num_nodes),wrap(y1,num_elms)).template send<
          NeighborMsg,&TestColl::informNeighbor
        >(idx, msg->do_check_);
      }
    }
  }

  void informNeighbor(NeighborMsg* m) {
    auto proxy = this->getCollectionProxy();
    auto sidx = m->idx_;
    proxy(sidx).template send<TestMsg,&TestColl::getBack>(m->do_check_);
    if (m->do_check_ and m->getHops() > 1) {
      vt_print(gen, "found long hop message hops={}\n", m->getHops());
    }
    EXPECT_TRUE(not m->do_check_ or m->getHops() <= 1);
  }

  void getBack(TestMsg* m) {
    if (m->do_check_ and m->getHops() > 1) {
      vt_print(gen, "found long hop message hops={}\n", m->getHops());
    }
    EXPECT_TRUE(not m->do_check_ or m->getHops() <= 1);
  }

  void cont(TestMsg* m) {

  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    Collection<TestColl,Index2D>::serialize(s);
    s | vec;
  }

  std::vector<double> vec;
};

TEST_F(TestHops, test_hops_1) {
  auto num_nodes = theContext()->getNumNodes();
  auto this_node = theContext()->getNode();

  auto const& range = vt::Index2D((int)num_nodes, (int)num_elms);
  auto proxy = theCollection()->constructCollective<TestColl>(range);

  for (int i = 0; i < 100; i++) {
    if (this_node == 0) {
      vt_print(gen, "Doing work stage 1 for iter={}\n", i);
    }
    runInEpochCollective([&]{
      if (this_node == 0) {
        proxy.broadcast<TestColl::TestMsg,&TestColl::doWork>(false);
      }
    });
    if (this_node == 0) {
      vt_print(gen, "Doing work stage 2 for iter={}\n", i);
    }
    runInEpochCollective([&]{
      if (this_node == 0) {
        proxy.broadcast<TestColl::TestMsg,&TestColl::doWork>(true);
      }
    });
    if (this_node == 0) {
      vt_print(gen, "Running LB for iter={}\n", i);
    }

    thePhase()->nextPhaseCollective();
  }

}

}}} /* end namespace vt::tests::unit */
