/*
//@HEADER
// *****************************************************************************
//
//                             collection_insert.cc
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

#include "vt/transport.h"

#include <cstdlib>

using namespace ::vt;
using namespace ::vt::collective;
using namespace ::vt::mapping;

static constexpr int32_t const default_num_elms = 64;

struct InsertCol : InsertableCollection<InsertCol,Index1D> {
  InsertCol()
    : InsertableCollection<InsertCol,Index1D>()
  {
    auto const& this_node = theContext()->getNode();
    ::fmt::print("{}: constructing: idx={}\n", this_node, getIndex().x());
 }
};

struct TestMsg : CollectionMessage<InsertCol> { };

static void work(TestMsg* msg, InsertCol* col) {
  auto const& this_node = theContext()->getNode();
  ::fmt::print("work: node={}, idx={}\n", this_node, col->getIndex().x());
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& this_node = theContext()->getNode();

  int32_t num_elms = default_num_elms;

  if (argc > 1) {
    num_elms = atoi(argv[1]);
  }

  auto const epoch = theTerm()->makeEpochCollective();
  theMsg()->setGlobalEpoch(epoch);

  if (this_node == 0) {
    auto const& range = Index1D(num_elms);
    auto proxy = theCollection()->construct<InsertCol>(range);

    theTerm()->addActionEpoch(epoch,[=]{
      ::fmt::print("broadcasting\n");
      auto msg = makeSharedMessage<TestMsg>();
      proxy.broadcast<TestMsg,work>(msg);
    });
    for (int i = 0; i < range.x() / 2; i++) {
      proxy[i].insert(i % 2);
      /*
       * Alternative syntax:
       *   theCollection()->insert<InsertCol>(proxy, Index1D(i));
       */
    }
    theTerm()->finishedEpoch(epoch);
    ::fmt::print("calling finished insert\n");
    proxy.finishedInserting([=]{
      ::fmt::print("insertions are finished\n");
      for (int i = range.x()/2; i < range.x(); i++) {
        proxy[i].insert(i % 2);
      }
      proxy.finishedInserting([]{
        ::fmt::print("insertions are finished2\n");
      });
    });
  } else {
    theTerm()->finishedEpoch(epoch);
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
