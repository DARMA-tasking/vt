/*
//@HEADER
// *****************************************************************************
//
//                          test_collectives_reduce.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#include <gtest/gtest.h>

#include "test_parallel_harness.h"
#include "data_message.h"
#include "test_collectives_reduce.h"
#include "test_helpers.h"

#include <vt/transport.h>

namespace vt { namespace tests { namespace unit {

TEST_F(TestReduce, test_reduce_op) {
  auto const my_node = theContext()->getNode();
  auto const root = 0;

  auto msg = makeMessage<MyReduceMsg>(my_node);
  vt_debug_print(normal, reduce, "msg->num={}\n", msg->num);
  theCollective()->global()->reduce<MyReduceMsg, reducePlus>(root, msg.get());
}

using ReduceMsg = vt::collective::ReduceTMsg<int>;

struct MyObjGroup {
  void handler(ReduceMsg* msg) {
    fmt::print("Reduce complete at {} value {}\n", vt::theContext()->getNode(), msg->getVal());
  }
};

vt::objgroup::proxy::Proxy<MyObjGroup> objgroup_proxy;

struct Hello : vt::Collection<Hello, vt::Index1D> {

  using TestMsg = vt::CollectionMessage<Hello>;

  void doWork(TestMsg* msg) {
    fmt::print("{}: Hello from {}\n", vt::theContext()->getNode(), this->getIndex());

    // Get the proxy for the collection
    auto proxy = this->getCollectionProxy();

    // Create a callback for when the reduction finishes
    auto cb = vt::theCB()->makeBcast<MyObjGroup,ReduceMsg,&MyObjGroup::handler>(objgroup_proxy);

    // Create and send the reduction message holding an int
    auto red_msg = vt::makeMessage<ReduceMsg>(this->getIndex().x());
    proxy.reduce<vt::collective::PlusOp<int>>(red_msg.get(),cb);
  }
};

vt::NodeType map(vt::Index1D* idx, vt::Index1D* max_idx, vt::NodeType num_nodes) {
  return (idx->x() % (num_nodes-1))+1;
}

TEST_F(TestReduce, test_reduce_with_no_elements_on_root_rank) {
  SET_MIN_NUM_NODES_CONSTRAINT(2);

  vt::NodeType this_node = vt::theContext()->getNode();

  int32_t num_elms = 16;

  objgroup_proxy = vt::theObjGroup()->makeCollective<MyObjGroup>();

  auto range = vt::Index1D(num_elms);
  auto proxy = vt::makeCollection<Hello>()
    .bounds(range)
    .mapperFunc<map>()
    .bulkInsert()
    .wait();

  if (this_node == 0) {
    proxy.broadcast<Hello::TestMsg,&Hello::doWork>();
  }
}

}}} // end namespace vt::tests::unit
