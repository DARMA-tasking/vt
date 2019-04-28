/*
//@HEADER
// ************************************************************************
//
//                         test_slice.cc
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

#include "vt/transport.h"
#include "test_parallel_harness.h"

namespace vt { namespace tests { namespace unit {

struct TestSlicing : TestParallelHarness {

  struct MyCol : vt::Collection<MyCol, vt::Index1D> {};
  using ViewMsg = vt::CollectViewMessage<MyCol>;
  using AckMsg  = vt::Message;

  struct ElemMsg : ViewMsg {
    using Index = ViewMsg::IndexType;

    ElemMsg(int in_idx, int in_size)
      : index_(in_idx),
        size_(in_size)
    {}

    Index const& getIndex() const { return index_; }
    int getSize() const { return size_; }

  private:
    Index index_ = {};
    int size_ = 0;
  };

  static vt::NodeType total_received;
  static vt::NodeType root;

  // index filtering function for the slice
  static bool filter(vt::Index1D* idx) {
    return idx->x() % 2 == 0;
  }

  static void acknowledged(AckMsg* msg) {
    EXPECT_EQ(vt::theContext()->getNode(), root);
    total_received++;
  }

  // the slice broadcast handler
  static void checkElem(ViewMsg* msg, MyCol* col) {

    auto const& node = vt::theContext()->getNode();
    auto const& elem = col->getIndex().x();
    EXPECT_TRUE(elem % 2 == 0);

    if (node == root) {
      total_received++;
    } else {
      auto ack = vt::makeSharedMessage<AckMsg>();
      vt::theMsg()->sendMsg<AckMsg, acknowledged>(root, ack);
    }

    debug_print(
      vrt_coll, node,
      "rank:{} got mycol[{}]\n", node, elem
    );
  }

  static void checkIndex(ElemMsg* msg, MyCol* col) {

    auto const& rel_idx = msg->getIndex().x();
    auto const& abs_idx = col->getIndex().x();
    EXPECT_EQ(rel_idx * 2, abs_idx);

    debug_print(
      vrt_coll, node,
      "rank:{} slice[{}] -> mycol[{}], size={}\n",
      vt::theContext()->getNode(), rel_idx, abs_idx, msg->getSize()
    );
  }
};

/*static*/ vt::NodeType TestSlicing::total_received = 0;
/*static*/ vt::NodeType TestSlicing::root = 0;

TEST_F(TestSlicing, test_slice) {

  auto const node  = vt::theContext()->getNode();
  auto const epoch = vt::theTerm()->makeEpochCollective();

  vt::CollectionProxy<MyCol> slice {};

  if (node == root) {
    // create collection, halve it and then keep only even elements
    auto const nb_nodes = vt::theContext()->getNumNodes();
    auto const range    = vt::Index1D(nb_nodes * 4);
    auto const half     = vt::Index1D(range.x() / 2);

    // build the distributed collection and get a proxy on it
    auto proxy = vt::theCollection()->construct<MyCol>(range);
    // create a view to the slice
    slice = proxy.slice<&filter>(range, half, epoch);
    // create a message and broadcast to each slice element
    slice.broadcast<ViewMsg, &checkElem>();

    // check acknowledged slice elem count
    vt::theTerm()->addAction(epoch, [=]{
      EXPECT_EQ(total_received, nb_nodes);
    });
  }

  vt::theCollective()->barrier();
  vt::theTerm()->finishedEpoch(epoch);
}

TEST_F(TestSlicing, test_relative_indexing) {

  auto const node  = vt::theContext()->getNode();
  auto const epoch = vt::theTerm()->makeEpochCollective();

  vt::CollectionProxy<MyCol> slice {};

  if (node == root) {
    auto const nb_nodes = vt::theContext()->getNumNodes();
    auto const range    = vt::Index1D(nb_nodes * 4);
    // create the distributed collection
    auto proxy = vt::theCollection()->construct<MyCol>(range);
    // create a view to the slice
    slice = proxy.slice<&filter>(range, range, epoch);
    // retrieve the actual size of the slice
    auto const size = slice.size();
    // each slice element sends a message
    for (int i = 0; i < size; ++i) {
      slice[i].send<ElemMsg, &checkIndex>(i, size);
    }
  }

  vt::theCollective()->barrier();
  vt::theTerm()->finishedEpoch(epoch);
}

}}} // end namespace vt::tests::unit