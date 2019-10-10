/*
//@HEADER
// *****************************************************************************
//
//                             simple_collection.cc
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

static constexpr std::size_t const default_num_elms = 64;

using IndexType = IdxType1D<std::size_t>;

struct TestColl : Collection<TestColl,IndexType> {
  TestColl() = default;

  virtual ~TestColl() {
    vtAssert(counter_ == 1, "Must be equal");
  }

  struct TestMsg : CollectionMessage<TestColl> { };

  void doWork(TestMsg* msg) {
    counter_++;
  }

private:
  int32_t counter_ = 0;
};

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& this_node = theContext()->getNode();

  int32_t num_elms = default_num_elms;

  if (argc > 1) {
    num_elms = atoi(argv[1]);
  }

  if (this_node == 0) {
    using BaseIndexType = typename IndexType::DenseIndexType;
    auto const& range = IndexType(static_cast<BaseIndexType>(num_elms));
    auto proxy = theCollection()->construct<TestColl>(range);
    proxy.broadcast<TestColl::TestMsg,&TestColl::doWork>();
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
