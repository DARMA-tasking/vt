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
#include <vt/vrt/collection/balance/model/per_collection.h>
#include <vt/vrt/collection/balance/model/composed_model.h>
#include <vt/vrt/collection/balance/model/load_model.h>

#include <memory>
#include <cinttypes>

inline vt::NodeType empireMap(vt::Index2D* idx, vt::Index2D*, vt::NodeType) {
  return idx->x();
}

struct Hello : vt::Collection<Hello, vt::Index2D> {
  using TestMsg = vt::CollectionMessage<Hello>;

  void timestep(TestMsg* msg) { }
};

using vt::vrt::collection::balance::ComposedModel;
using vt::vrt::collection::balance::LoadModel;
using vt::vrt::collection::balance::ElementIDType;
using vt::vrt::collection::balance::PhaseOffset;
using vt::vrt::collection::balance::PerCollection;

struct FileModel : ComposedModel {

  FileModel(std::shared_ptr<LoadModel> in_base, std::string const& filename)
    : vt::vrt::collection::balance::ComposedModel(in_base)
  {
    parseFile(filename);
  }

  vt::TimeType getWork(ElementIDType elmid, PhaseOffset offset) override {
    //ignore offset for now
    //vt_print(gen, "getWork {} phase={}\n", elmid, getNumCompletedPhases());
    auto const phase = getNumCompletedPhases(); // FIXME: off by 1 error
    auto iter = loads.find(phase);
    vtAbortIf(iter == loads.end(), "Must have phase in history");
    auto elmiter = iter->second.find(elmid);
    if (elmiter == iter->second.end()) {
      vt_print(gen, "could not find elm_id={}\n", elmid);
    }
    vtAbortIf(elmiter == iter->second.end(), "Must have elm ID in history");
    return elmiter->second;
  }

  void parseFile(std::string const& file) {
    std::FILE *f = std::fopen(file.c_str(), "r");
    vtAbortIf(not f, "File opening failed");

    int phase = 0;
    ElementIDType elm_id = 0;
    vt::TimeType load = 0.;

    vt_print(gen, "parsing file {}\n", file);

    while (fscanf(f, "%d, %" PRIu64 ", %lf", &phase, &elm_id, &load) == 3) {
      //vt_print(gen, "reading in loads for elm={}, phase={}\n", elm_id, phase);
      loads[phase][elm_id] = load;
    }
  }

private:
  std::unordered_map<int, std::unordered_map<ElementIDType, vt::TimeType>> loads;
};

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  vtAbortIf(
    argc != 4, "Must have two arguments: <num_elms per rank>, <ts>, <file>"
  );

  int32_t num_elms = atoi(argv[1]);
  int32_t ts = atoi(argv[2]);
  std::string file = std::string{argv[3]};

  auto const nranks = vt::theContext()->getNumNodes();
  auto const node = vt::theContext()->getNode();

  auto range = vt::Index2D(static_cast<int>(nranks), num_elms);
  auto proxy = vt::theCollection()->constructCollective<Hello,empireMap>(range);

  auto base = vt::theLBManager()->getBaseLoadModel();
  auto per_col = std::make_shared<PerCollection>(base);

  auto proxy_bits = proxy.getProxy();

  auto node_filename = fmt::format("{}.{}.out", file, node);

  per_col->addModel(proxy_bits, std::make_shared<FileModel>(base, node_filename));

  vt::theLBManager()->setLoadModel(per_col);

  for (int i = 0; i < ts; i++) {
    // Delete this?
    proxy.broadcastCollective<Hello::TestMsg, &Hello::timestep>();

    vt::thePhase()->nextPhaseCollective();
  }

  vt::finalize();

  return 0;
}
