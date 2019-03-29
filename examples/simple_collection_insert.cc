/*
//@HEADER
// ************************************************************************
//
//                          simple_collection_insert.cc
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

using namespace ::vt;
using namespace ::vt::collective;
using namespace ::vt::mapping;

static constexpr std::size_t const default_num_elms = 64;

using IndexType = IdxType1D<std::size_t>;

struct TestColl : Collection<TestColl,IndexType> {
  TestColl() = default;

  virtual ~TestColl() {
    // auto num_nodes = theContext()->getNumNodes();
    // vtAssertInfo(
    //   counter_ == num_nodes, "Must be equal",
    //   counter_, num_nodes, getIndex(), theContext()->getNode()
    // );
    fmt::print("{}: destroying TestColl\n", getIndex());
  }

  struct TestMsg : CollectionMessage<TestColl> { };

  void doWork(TestMsg* msg) {
    auto const& this_node = theContext()->getNode();
    counter_++;
    ::fmt::print(
      "{}: doWork: idx={}, cnt={}\n", this_node, getIndex().x(), counter_
    );
  }

private:
  int32_t counter_ = 0;
};

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  int32_t num_elms = default_num_elms;

  if (argc > 1) {
    num_elms = atoi(argv[1]);
  }

  using BaseIndexType = typename IndexType::DenseIndexType;
  auto const& range = IndexType(static_cast<BaseIndexType>(num_elms));
  auto token = theCollection()->constructInsert<TestColl>(range);

  for (size_t i = 0; i < default_num_elms; i++) {
    if (i % num_nodes == static_cast<size_t>(this_node)) {
      fmt::print("node={}: inserting {}\n", this_node, i);
      token[i].insert();
    }
  }

  fmt::print("finishedInsert: node={}: finished\n", this_node);
  auto proxy = theCollection()->finishedInsert(std::move(token));

  if (this_node == 1) {
    auto msg = makeSharedMessage<TestColl::TestMsg>();
    proxy[2].send<TestColl::TestMsg,&TestColl::doWork>(msg);
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
