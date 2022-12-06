/*
//@HEADER
// *****************************************************************************
//
//                                simulate_lb.cc
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
#include <vt/vrt/collection/balance/lb_data_holder.h>
#include <vt/utils/json/json_reader.h>

using LBDataHolder = vt::vrt::collection::balance::LBDataHolder;
using ElementIDStruct = vt::vrt::collection::balance::ElementIDStruct;

namespace vt {

std::unique_ptr<LBDataHolder> readInData(std::string const& file_name) {
  using vt::util::json::Reader;
  Reader r{file_name};
  auto j = r.readFile();
  auto d = std::make_unique<LBDataHolder>(*j);
  return d;
}

} /* end namespace vt */

struct SimCol : vt::Collection<SimCol, vt::Index1D> {
  using Msg = vt::CollectionMessage<SimCol>;
  void handler(Msg* m) {
    vt_print(gen, "handler: idx={}\n", getIndex());
  }
};

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  std::vector<LBDataHolder> lb_data;

  double read_time = 0;

  auto this_node = vt::theContext()->getNode();
  auto num_nodes = vt::theContext()->getNumNodes();

  auto t1 = vt::timing::getCurrentTime();
  for (int i = 1; i < argc; i++) {
    std::string const filename = std::string{argv[i]};
    if (this_node == 0) {
      fmt::print("Reading in filename={}\n", filename);
    }
    auto p = vt::readInData(filename);
    lb_data.push_back(*p);
  }
  read_time = vt::timing::getCurrentTime() - t1;

  fmt::print("Result: {:0.2f}s read\n", read_time);

  int offset = 0;
  for (int i = 0; i < static_cast<int>(this_node); i++) {
    offset += static_cast<int>(lb_data[i].node_data_[0].size());
  }

  int total = 0;
  for (int i = 0; i < static_cast<int>(num_nodes); i++) {
    total += static_cast<int>(lb_data[i].node_data_[0].size());
  }

  auto& h = lb_data[this_node];
  auto& m = h.node_data_[0];

  std::vector<std::tuple<vt::Index1D, std::unique_ptr<SimCol>>> elms;
  for (int j = 0; j < static_cast<int>(m.size()); j++) {
    elms.emplace_back(vt::Index1D{offset+j}, std::make_unique<SimCol>());
  }

  vt::Index1D range{total};
  auto proxy = vt::makeCollection<SimCol>("simcol")
    .bounds(range)
    .listInsertHere(std::move(elms))
    .wait();
  proxy.broadcastCollective<typename SimCol::Msg, &SimCol::handler>();

  vt::thePhase()->nextPhaseCollective();
  proxy.broadcastCollective<typename SimCol::Msg, &SimCol::handler>();

  vt::finalize();
  return 0;
}
