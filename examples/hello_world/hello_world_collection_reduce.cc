/*
//@HEADER
// *****************************************************************************
//
//                       hello_world_collection_reduce.cc
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

#include <vt/transport.h>

/// [Hello world reduce collection]
struct Hello : vt::Collection<Hello, vt::Index1D> {
  using ReduceMsg = vt::collective::ReduceTMsg<int>;

  void done(ReduceMsg* msg) {
    fmt::print("Reduce complete at {} value {}\n", this->getIndex(), msg->getVal());
  }

  using TestMsg = vt::CollectionMessage<Hello>;

  void doWork(TestMsg* msg) {
    fmt::print("Hello from {}\n", this->getIndex());

    // Get the proxy for the collection
    auto proxy = this->getCollectionProxy();

    // Create a callback for when the reduction finishes
    auto cb = vt::theCB()->makeSend<Hello,ReduceMsg,&Hello::done>(proxy(2));

    // Create and send the reduction message holding an int
    auto red_msg = vt::makeMessage<ReduceMsg>(this->getIndex().x());
    proxy.reduce<vt::collective::PlusOp<int>>(red_msg.get(),cb);
  }
};

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  vt::NodeType this_node = vt::theContext()->getNode();

  int32_t num_elms = 16;
  if (argc > 1) {
    num_elms = atoi(argv[1]);
  }

  if (this_node == 0) {
    auto range = vt::Index1D(num_elms);
    auto proxy = vt::theCollection()->construct<Hello>(range);
    proxy.broadcast<Hello::TestMsg,&Hello::doWork>();
  }

  vt::finalize();

  return 0;
}
/// [Hello world reduce collection]
