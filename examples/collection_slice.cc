/*
//@HEADER
// ************************************************************************
//
//                       collection_slice.cc
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

namespace example {
// default collection size
static constexpr int const default_size = 16;

// the collection to be sliced
struct MyCol : vt::Collection<MyCol, vt::Index1D> {

  MyCol() {
    fmt::print(
      "building collection on node={}: idx.x()={}\n",
      vt::theContext()->getNode(), getIndex().x()
    );
  }
};

// collection view message type
using ViewMsg = vt::CollectViewMessage<MyCol>;

// view element message type
struct ElemMsg : ViewMsg {
  using Index = ViewMsg::IndexType;

  ElemMsg(int in_idx) : index_(in_idx) {}
  Index const& getIndex() const { return index_; }

private:
  Index index_ = {};
};

// index filtering for the slice
static bool filter(vt::Index1D* idx) {
  return idx->x() % 2 == 0;
}

// the slice broadcast handler
static void showElem(ViewMsg* msg, MyCol* col) {

  auto const& node = vt::theContext()->getNode();
  auto const& elem = col->getIndex().x();
  fmt::print("rank:{} got mycol[{}]\n", node, elem);
}

static void showIndex(ElemMsg* msg, MyCol* col) {

  auto const& node = vt::theContext()->getNode();
  auto const& rel_index = msg->getIndex();
  auto const& abs_index = col->getIndex();

  fmt::print(
    "rank:{} slice[{}] -> mycol[{}]\n",
    node, rel_index.x(), abs_index.x()
  );
}

} // end namespace example


int main(int argc, char** argv) {
  // initialize the runtime
  vt::CollectiveOps::initialize(argc, argv);

  auto const root  = vt::NodeType(0);
  auto const node  = vt::theContext()->getNode();
  auto const epoch = vt::theTerm()->makeEpochCollective();

  if (node == root) {

    auto const range = vt::Index1D(example::default_size);
    auto const half  = vt::Index1D(range.x() / 2);
    auto const elems = int(half.x() / 2);

    fmt::print("root: build collection, halve it, then keep only even elems\n");

    // create the distributed collection
    auto proxy = vt::theCollection()->construct<example::MyCol>(range);
    // create a view to the sliced collection
    auto slice = proxy.slice<&example::filter>(range, half, epoch);
    // create a message and broadcast to each slice element
    slice.broadcast<example::ViewMsg, &example::showElem>();
    // each element of the slice sends a message
    for (int i = 0; i < elems; ++i) {
      slice[i].send<example::ElemMsg, &example::showIndex>(i);
    }
  }

  vt::theCollective()->barrier();
  vt::theTerm()->finishedEpoch(epoch);

  // spin until termination
  while (not vt::rt->isTerminated()) {
    vt::runScheduler();
  }

  // finalize the runtime and exit
  vt::CollectiveOps::finalize();
  return EXIT_SUCCESS;
}