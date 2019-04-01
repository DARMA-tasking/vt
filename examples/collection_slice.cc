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
static int received = 0;

// the collection to be sliced
struct MyCol : vt::Collection<MyCol, vt::Index1D> {

  MyCol() {
    fmt::print(
      "constructing example::MyCol on node={}: idx.x()={}\n",
      vt::theContext()->getNode(), getIndex().x()
    );
  }
};

// collection message
using ViewMsg = vt::CollectViewMessage<MyCol, vt::Index1D>;

// the slice view relative indexing function
static vt::Index1D filter(vt::Index1D* idx) {
  const auto& cur = idx->x();
  return vt::Index1D(cur % 4 == 0 ? (cur / 4) : -1);
}

// the slice broadcast handler
static void printElem(ViewMsg* msg, MyCol* col) {

  fmt::print("ok cool, I will print new index here\n");

//  auto const& my_node = vt::theContext()->getNode();
//  auto const& proxy   = msg->getProxy();
//  auto const& section = col->getView<vt::Index1D>(proxy);
//  auto const& index   = col->getIndex();
//
//  fmt::print(
//    "node {}: print elem: index.x():{}\n",
//    my_node, index.x(), section[index.x()]
//  );

  received++;
}

} // end namespace


int main(int argc, char** argv) {
  // initialize the runtime
  vt::CollectiveOps::initialize(argc, argv);

  auto const my_node = vt::theContext()->getNode();
  auto const root = vt::NodeType(0);

  auto const epoch = vt::theTerm()->makeEpochCollective();

  if (my_node == root) {
    auto range = vt::Index1D(example::default_size);
    auto proxy = vt::theCollection()->construct<example::MyCol>(range);

    // slice the initial range
    auto slice = vt::Index1D(range.x() / 2);
    // create a view to the sliced collection
    auto section = proxy.slice<vt::Index1D, &example::filter>(range, slice, epoch);
    // broadcast to each slice element
    auto virtual_proxy = section.getProxy();
    section.broadcast<example::ViewMsg, &example::printElem>(virtual_proxy);
  }

  vt::theCollective()->barrier();
  vt::theTerm()->finishedEpoch(epoch);

  // spin until termination
  while (not vt::rt->isTerminated()) {
    vt::runScheduler();
  }

  fmt::print("node:{} received:{}\n", my_node, example::received);

  // finalize the runtime and exit
  vt::CollectiveOps::finalize();
  return EXIT_SUCCESS;
}