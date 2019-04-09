/*
//@HEADER
// ************************************************************************
//
//                          simple_collection.cc
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

#include <cstdlib>
#include <cassert>

static constexpr std::size_t const default_collection_size = 32;
static int nb_elems = 0;

using IndexType = vt::IdxType1D<std::size_t>;

struct MyCol : vt::Collection<MyCol,IndexType> {
  MyCol() {
    fmt::print(
      "node[{}]: building mycol: idx.x()={}\n",
      vt::theContext()->getNode(), getIndex().x()
    );
  }

  virtual ~MyCol() {
    auto const nb_nodes = vt::theContext()->getNumNodes();
    auto const expected = static_cast<int>(nb_elems / nb_nodes);
    vtAssert(counter == expected, "Must be equal");
  }

  struct ColMsg : vt::CollectionMessage<MyCol> {};

  static void doWork(ColMsg* msg, MyCol* col) {
    counter++;
    fmt::print(
      "node[{}]: do work on mycol[{}]\n",
      vt::theContext()->getNode(), col->getIndex().x()
    );
  }

  static int counter;
};

/*static*/ int MyCol::counter = 0;

int main(int argc, char** argv) {
  vt::CollectiveOps::initialize(argc, argv);

  auto const root = vt::NodeType(0);
  auto const node = vt::theContext()->getNode();

  nb_elems = (argc > 1 ? std::atoi(argv[1]) : default_collection_size);
  vtAssert(nb_elems > 0, "Invalid number of elems");

  if (node == root) {
    using BaseIndexType = typename IndexType::DenseIndexType;
    auto const& range = IndexType(static_cast<BaseIndexType>(nb_elems));
    auto proxy = vt::theCollection()->construct<MyCol>(range);
    proxy.broadcast<MyCol::ColMsg, &MyCol::doWork>();
  }

  while (not vt::rt->isTerminated()) {
    vt::runScheduler();
  }

  vt::CollectiveOps::finalize();
  return EXIT_SUCCESS;
}
