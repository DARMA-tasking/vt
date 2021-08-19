/*
//@HEADER
// *****************************************************************************
//
//                           insertable_collection.cc
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

static constexpr int32_t const default_num_elms = 64;

struct InsertCol : vt::Collection<InsertCol, vt::Index1D> {
  InsertCol() {
    vt::NodeType this_node = vt::theContext()->getNode();
    ::fmt::print("{}: constructing: idx={}\n", this_node, getIndex().x());
  }
};

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  if(vt::theContext()->getNumNodes() < 2){
    vt::finalize();
    return 0;
  }

  vt::NodeType this_node = vt::theContext()->getNode();
  vt::NodeType num_nodes = vt::theContext()->getNumNodes();

  int32_t num_elms = default_num_elms;
  if (argc > 1) {
    num_elms = atoi(argv[1]);
  }

  auto range = vt::Index1D(num_elms);
  auto proxy = vt::makeCollection<InsertCol>()
    .dynamicMembership(true)
    .wait();

  {
    auto token = proxy.beginModification();

    for (int i = 0; i < range.x() / 2; i++) {
      if (i % num_nodes == this_node) {
        proxy[i].insertAt(token, i % 2);
      }
    }

    proxy.finishModification(std::move(token));
    fmt::print("called finishedModification\n");
  }

  {
    auto token = proxy.beginModification();

    for (int i = range.x()/2; i < range.x(); i++) {
      if (i % num_nodes == this_node) {
        proxy[i].insertAt(token, i % 2);
      }
    }
    proxy.finishModification(std::move(token));
    fmt::print("called finishedModification (2)\n");
  }


  vt::finalize();

  return 0;
}
