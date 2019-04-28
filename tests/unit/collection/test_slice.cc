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

  static vt::NodeType root;
  static vt::NodeType nb_nodes;
  static vt::NodeType total_received;

  void SetUp() override {
    TestParallelHarness::SetUp();
    nb_nodes = vt::theContext()->getNumNodes();
    EXPECT_TRUE(nb_nodes > 1);

    // reset counter
    total_received = 0;
    vt::theCollective()->barrier();
  }

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

  template <bool nested = false>
  static void checkIndex(ElemMsg* msg, MyCol* col) {

    auto const& rel_idx = msg->getIndex().x();
    auto const& abs_idx = col->getIndex().x();
    auto const offset = (nested ? 4 : 2);
    EXPECT_EQ(rel_idx * offset, abs_idx);

    debug_print(
      vrt_coll, node,
      "rank:{} slice[{}] -> mycol[{}], size={}\n",
      vt::theContext()->getNode(), rel_idx, abs_idx, msg->getSize()
    );
  }
};

/*static*/ vt::NodeType TestSlicing::root = 0;
/*static*/ vt::NodeType TestSlicing::nb_nodes = 0; // intialized in SetUp()
/*static*/ vt::NodeType TestSlicing::total_received = 0;

TEST_F(TestSlicing, test_collect_slice) {

  EXPECT_EQ(total_received, 0);

  auto const node  = vt::theContext()->getNode();
  auto const epoch = vt::theTerm()->makeEpochCollective();

  vt::CollectionProxy<MyCol> slice {};

  if (node == root) {
    // create collection, halve it and then keep only even elements
    auto const range = vt::Index1D(nb_nodes * 4);
    auto const half  = vt::Index1D(range.x() / 2);

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
    auto const range = vt::Index1D(nb_nodes * 4);
    // create the distributed collection
    auto proxy = vt::theCollection()->construct<MyCol>(range);
    // create a view to the slice
    slice = proxy.slice<&filter>(range, range, epoch);
    // retrieve the actual size of the slice
    auto const size = slice.size();
    // each slice element sends a message
    for (int i = 0; i < size; ++i) {
      slice[i].send<ElemMsg, &checkIndex<false>>(i, size);
    }
  }

  vt::theCollective()->barrier();
  vt::theTerm()->finishedEpoch(epoch);
}

TEST_F(TestSlicing, test_chained_slicing) {

  EXPECT_EQ(total_received, 0);

  auto const node  = vt::theContext()->getNode();
  auto const epoch = vt::theTerm()->makeEpochCollective();

  vt::CollectionProxy<MyCol> section {};
  vt::CollectionProxy<MyCol> nested {};

  if (node == root) {

    auto const col_range = vt::Index1D(nb_nodes * 4);
    auto const sec_range = vt::Index1D(col_range.x() / 2);
    auto const new_range = vt::Index1D(sec_range.x() / 2);

    // 1. create a slice of a slice
    auto proxy = vt::theCollection()->construct<MyCol>(col_range);
    section = proxy.slice<&filter>(col_range, sec_range, epoch);
    nested = section.slice<&filter>(sec_range, new_range, epoch);
    nested.broadcast<ViewMsg, &checkElem>();

    // 2. check relative indexing
    auto const size = nested.size();
    for (int i = 0; i < size; ++i) {
      nested[i].send<ElemMsg, &checkIndex<true>>(i, size);
    }

    // 3. check element count
    vt::theTerm()->addAction(epoch, [=]{
      auto const expected = nb_nodes / 2;
      EXPECT_EQ(size, expected);
      EXPECT_EQ(total_received, expected);
    });
  }

  vt::theCollective()->barrier();
  vt::theTerm()->finishedEpoch(epoch);
}

}}} // end namespace vt::tests::unit