/*
//@HEADER
// *****************************************************************************
//
//                          lb_data_file_generator.cc
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

struct WorkCol : vt::Collection<WorkCol, vt::Index1D> {
  void someWork(int subphase) {
    this->lb_data_.setSubPhase(subphase);

    // Do some work
    double someVal = 0.1f;
    double someVal2 = 1.0f;
    for (int i = 0; i < 100000; i++) {
      someVal *= i + someVal2;
      someVal2 += someVal;
    }

    // Generate data for communications field
    vt::NodeType this_node = vt::theContext()->getNode();
    auto proxy = this->getCollectionProxy();
    proxy(this_node).send<&WorkCol::receiveVal>(someVal2);
  }

  void receiveVal(int) { }
};

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  int32_t num_phases = argc > 1 ? atoi(argv[1]) : 1;
  int32_t num_subphases = argc > 2 ? atoi(argv[2]) : 1;
  int32_t num_elms = argc > 3 ? atoi(argv[3]) : 1;

  if (vt::theContext()->getNode() == 0) {
    fmt::print(
      "lb_data_file_generator: num_phases={}, num_subphases={}, num_elms={}, "
      "\n",
      num_phases, num_subphases, num_elms);
  }

  auto range = vt::Index1D(num_elms);
  auto proxy = vt::makeCollection<WorkCol>("examples_lb_data_file_generator")
                 .bounds(range)
                 .bulkInsert()
                 .wait();

  for (int32_t phase = 0; phase < num_phases; phase++) {
    for (int32_t sub = 0; sub < num_subphases; sub++) {
      vt::runInEpochCollective(
        [=] { proxy.broadcastCollective<&WorkCol::someWork>(sub); });
    }

    vt::thePhase()->nextPhaseCollective();
  }

  vt::finalize();
  return 0;
}