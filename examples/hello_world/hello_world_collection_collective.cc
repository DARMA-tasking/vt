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

#include <vt/transport.h>

/// [Hello world collective collection]
struct Hello : vt::Collection<Hello, vt::Index1D> {
  Hello() = default;

  virtual ~Hello() {
    vt::NodeType num_nodes = vt::theContext()->getNumNodes();
    vtAssert(counter_ == num_nodes, "Should receive # nodes broadcasts");
  }

  using TestMsg = vt::CollectionMessage<Hello>;

  void doWork(TestMsg* msg) {
    counter_++;
    fmt::print("Hello from {}, counter_={}\n", this->getIndex().x(), counter_);
  }

private:
  int counter_ = 0;
};

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  int32_t num_elms = 16;
  if (argc > 1) {
    num_elms = atoi(argv[1]);
  }

  auto range = vt::Index1D(num_elms);
  auto proxy = vt::theCollection()->constructCollective<Hello>(
    range, [](vt::Index1D){ return std::make_unique<Hello>(); }
  );

  // All nodes send a broadcast to all elements
  proxy.broadcast<Hello::TestMsg,&Hello::doWork>();

  vt::finalize();

  return 0;
}
/// [Hello world collective collection]
