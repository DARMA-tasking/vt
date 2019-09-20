/*
//@HEADER
// *****************************************************************************
//
//                       simple_collection_collective.cc
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

static constexpr std::size_t const default_num_elms = 16;

struct TestColl : Collection<TestColl,vt::Index1D> {
  TestColl() = default;

  struct TestMsg : CollectionMessage<TestColl> {
    vt::Callback<vt::collective::ReduceNoneMsg> cb;
  };

  void doWork(TestMsg* msg) {
    auto proxy = this->getCollectionProxy();
    auto idx = this->getIndex().x();
    proxy[idx].template registerHandle<double*, &TestColl::handle>();

    auto prev = idx - 1 >= 0 ? idx - 1 : default_num_elms - 1;
    auto next = idx + 1 < default_num_elms ? idx + 1 : 0;
    proxy[prev].template connect<double*, &TestColl::handle>(vt::Index1D(idx));
    proxy[next].template connect<double*, &TestColl::handle>(vt::Index1D(idx));

    auto nmsg = vt::makeMessage<vt::collective::ReduceNoneMsg>();
    proxy.reduce(nmsg.get(),msg->cb);
  }

  void testHandle(TestMsg* msg) {
    auto proxy = this->getCollectionProxy();
    auto idx = this->getIndex().x();
    auto prev = idx - 1 >= 0 ? idx - 1 : default_num_elms - 1;
    auto next = idx + 1 < default_num_elms ? idx + 1 : 0;

    auto val = proxy[next].template atomicGetAccum<double*, &TestColl::handle>(idx);
    fmt::print("next idx={}: val={}\n", idx, val);

    auto var = proxy[prev].template atomicGetAccum<double*, &TestColl::handle>(idx);
    fmt::print("prev idx={}: val={}\n", idx, var);
  }

  vt::Handle<double*> handle;
  int32_t counter_ = 0;
};

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  int32_t num_elms = default_num_elms;

  if (argc > 1) {
    num_elms = atoi(argv[1]);
  }

  auto const& range = vt::Index1D(num_elms);
  auto proxy = theCollection()->constructCollective<TestColl>(
    range, [](vt::Index1D idx){
      return std::make_unique<TestColl>();
    }
  );

  if (vt::theContext()->getNode() == 0) {
    bool done = false;
    auto cb = theCB()->makeFunc<vt::collective::ReduceNoneMsg>(
      [&](vt::collective::ReduceNoneMsg*){
        done = true;
      }
    );
    auto msg = makeSharedMessage<TestColl::TestMsg>();
    msg->cb = cb;
    proxy.broadcast<TestColl::TestMsg,&TestColl::doWork>(msg);
    while (not done) vt::runScheduler();
    fmt::print("done!\n");
  }

  fmt::print("try barrier!\n");
  theCollective()->barrier();

  proxy.finishHandleCollective<double*, &TestColl::handle>();

  if (vt::theContext()->getNode() == 0) {
    auto msg = makeSharedMessage<TestColl::TestMsg>();
    proxy.broadcast<TestColl::TestMsg,&TestColl::testHandle>(msg);
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
