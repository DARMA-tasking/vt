/*
//@HEADER
// *****************************************************************************
//
//                         simple_collection_reduce.cc
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
#include <cassert>

using namespace ::vt;
using namespace ::vt::collective;
using namespace ::vt::mapping;

static constexpr std::size_t const default_num_elms = 8;

using TestReduceMsg = ReduceTMsg<int>;

struct TestColl : Collection<TestColl,Index1D> {
  TestColl() = default;

  virtual ~TestColl() = default;

  struct TestMsg : CollectionMessage<TestColl> { };

  void done(TestReduceMsg* msg) {
    auto const& this_node = theContext()->getNode();
    ::fmt::print("{}: done: idx={}, val={}\n", this_node, getIndex().x(), msg->getVal());
  }

  void doWork(TestMsg* msg) {
    auto const& this_node = theContext()->getNode();
    ::fmt::print("{}: doWork: idx={}\n", this_node, getIndex().x());

    auto proxy = this->getCollectionProxy();
    auto cb = theCB()->makeSend<TestColl,TestReduceMsg,&TestColl::done>(proxy(2));

    vtAssertExpr(cb.valid());
    auto rmsg = makeMessage<TestReduceMsg>();
    rmsg->getVal() = 10;
    proxy.reduce<collective::PlusOp<int>>(rmsg.get(),cb);
  }
};

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& this_node = theContext()->getNode();

  int32_t num_elms = default_num_elms;

  if (argc > 1) {
    num_elms = atoi(argv[1]);
  }

  if (this_node == 0) {
    using IndexType = typename TestColl::IndexType;
    using BaseIndexType = typename IndexType::DenseIndexType;
    auto const& range = IndexType(static_cast<BaseIndexType>(num_elms));
    auto proxy = theCollection()->construct<TestColl>(range);

    auto msg = makeSharedMessage<TestColl::TestMsg>();
    proxy.broadcast<TestColl::TestMsg,&TestColl::doWork>(msg);
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
