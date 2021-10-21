/*
//@HEADER
// *****************************************************************************
//
//                                hello_world.cc
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
#include <vt/datarep/dr.h>

struct HelloMsg : vt::Message {
  HelloMsg(vt::NodeType in_from, vt::DataRepIDType handle_id)
    : from(in_from),
      handle_id_(handle_id)
  { }

  vt::NodeType from = 0;
  vt::DataRepIDType handle_id_ = vt::no_datarep;
};

struct MyCol : vt::Collection<MyCol, vt::Index2D> {
  MyCol() {
    auto proxy = getCollectionProxy();
    auto idx = getIndex();
    vt_print(gen, "MyCol: {}\n", idx);
    //proxy[idx].makeIndexedHandle(test_dr);
    test_dr = vt::theDR()->makeIndexedHandle<std::vector<double>>(proxy[idx]);
    test_dr.publish(0, std::vector<double>{10.2, 32.4, 22.3 + idx.x() * idx.y()});
  }

  using TestMsg = vt::CollectionMessage<MyCol>;

  void doWork(TestMsg* msg) {
    auto proxy = getCollectionProxy();
    auto idx = getIndex();
    vt_print(gen, "Hello from {}\n", idx);
    vt::datarep::Reader<std::vector<double>, vt::Index2D> reader;
    auto idx_x = (idx.x() + 1) % 4;
    reader = vt::theDR()->makeIndexedReader<std::vector<double>>(proxy(idx_x, idx.y()));
    reader.fetch(0);
    vt::theSched()->runSchedulerWhile([&]{ return not reader.isReady(); });
    auto const& vec = reader.get(0);
    for (auto&& elm : *vec) {
      vt_print(gen, "idx={}, elm={}\n", idx, elm);
    }
    reader.release(0);
  }

  vt::datarep::DR<std::vector<double>, vt::Index2D> test_dr;
};


int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  // vt::NodeType this_node = vt::theContext()->getNode();
  vt::NodeType num_nodes = vt::theContext()->getNumNodes();

  if (num_nodes == 1) {
    return vt::rerror("requires at least 2 nodes");
  }

  auto range = vt::Index2D(4, 1);
  auto proxy = vt::makeCollection<MyCol>()
    .bounds(range)
    .bulkInsert()
    .wait();

  proxy.broadcastCollective<MyCol::TestMsg,&MyCol::doWork>();

  vt::finalize();

  return 0;
}
