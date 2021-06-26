/*
//@HEADER
// *****************************************************************************
//
//                   hello_world_collection_staged_insert.cc
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

/// [Hello world staged insert collection]
struct Hello : vt::Collection<Hello, vt::Index1D> {

  // Default constructor for migration
  Hello() = default;

  // Constructor used during insertion
  explicit Hello(std::string const& input_string)
    : in(input_string)
  { }

  virtual ~Hello() {
    vtAssert(counter_ == 1, "Must be equal");
  }

  using TestMsg = vt::CollectionMessage<Hello>;

  void doWork(TestMsg* msg) {
    counter_++;

    vt::NodeType this_node = vt::theContext()->getNode();
    fmt::print("{}: Hello from {}: {}\n", this_node, this->getIndex(), in);
  }

private:
  int counter_ = 0;
  std::string in;
};

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  vt::NodeType this_node = vt::theContext()->getNode();
  vt::NodeType num_nodes = vt::theContext()->getNumNodes();

  if(num_nodes < 2){
    vt::finalize();
    return 0;
  }

  int num_elms = 32;
  if (argc > 1) {
    num_elms = atoi(argv[1]);
  }

  auto range = vt::Index1D(num_elms);
  auto token = vt::theCollection()->constructInsert<Hello>(range);

  for (int i = 0; i < num_elms; i++) {
    // Insert even elements, round-robin the insertions from each node
    if ((i / 2) % num_nodes == this_node and i % 2 == 0) {
      auto str = fmt::format("inserted from {}", this_node);

      // Construct the i'th element on this node, passing str to the constructor
      token[i].insert(str);
    }
  }

  // Finish all inserts on this node by invalidating the insert token
  auto proxy = vt::theCollection()->finishedInsert(std::move(token));

  if (this_node == 1) {
    proxy.broadcast<Hello::TestMsg,&Hello::doWork>();
  }

  vt::finalize();

  return 0;
}
/// [Hello world staged insert collection]
