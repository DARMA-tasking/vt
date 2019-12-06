/*
//@HEADER
// *****************************************************************************
//
//                                stats_lb_reader.cc
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

#include "vt/config.h"
#include "vt/vrt/collection/balance/stats_lb_reader.h"
#include "vt/vrt/collection/balance/lb_invoke/invoke.h"
#include "vt/vrt/collection/balance/statsmaplb/statsmaplb.h"
#include "vt/vrt/collection/manager.h"
#include "vt/timing/timing.h"
#include "vt/configs/arguments/args.h"
#include "vt/runtime/runtime.h"

#include <cstdio>
#include <string>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "fmt/format.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

/*static*/
std::vector<std::map<ElementIDType,TimeType>>
  StatsLBReader::user_specified_map_changed_ = {};

/*static*/
StatsLBReader::VectorDiffPhase
StatsLBReader::phase_changed_map_ = {};

/*static*/ FILE* StatsLBReader::stats_file_ = nullptr;

/*static*/ bool StatsLBReader::created_dir_ = false;

/*static*/ objgroup::proxy::Proxy<StatsLBReader> StatsLBReader::proxy_ = {};

/*static*/ void StatsLBReader::init() {
  // Create the new class dedicated to the input reader
  StatsLBReader::proxy_ = theObjGroup()->makeCollective<StatsLBReader>();
  StatsLBReader::inputStatsFile();
  StatsLBReader::loadPhaseChangedMap();
  proxy_.get()->doReduce();
}

/*static*/ void StatsLBReader::destroy() {
  theObjGroup()->destroyCollective(StatsLBReader::proxy_);
}
/*static*/ void StatsLBReader::clearStats() {
  StatsLBReader::user_specified_map_changed_.clear();
}

/*static*/ void StatsLBReader::closeStatsFile() {
  if (stats_file_) {
    fclose(stats_file_);
    stats_file_  = nullptr;
  }
}

/*static*/ void StatsLBReader::inputStatsFile() {

  using ArgType = vt::arguments::ArgConfig;

  // todo if File exist
  auto const node = theContext()->getNode();
  auto const base_file = std::string(ArgType::vt_lb_stats_file_in);
  auto const dir = std::string(ArgType::vt_lb_stats_dir_in);
  auto const file = fmt::format("{}.{}.out", base_file, node);
  auto const file_name = fmt::format("{}/{}", dir, file);

  vt_print(lb, "inputStatFile: file={}, iter={}\n", file_name, 0);

  FILE *pFile = std::fopen (file_name.c_str(), "r");
  vtAssert(pFile, "File opening failed");

  // TODO loop on num_iters
  // Create a map for each different value of the first column
  // we should assume that every new value on the first column come
  // just after the end of the communication.
  // Finally the pattern is Load0, Com0, Load1, Com1, ..., LoadN, ComN
  // where 0, 1, ..., N are the values of the first column
  auto elements = std::map<ElementIDType,TimeType> ();

  // Load: Format of a line :size_t,ElementIDType,TimeType
  size_t c1;
  ElementIDType c2;
  TimeType c3;
  CommBytesType c4;
  using E = typename std::underlying_type<CommCategory>::type;
  E c5;
  char separator;
  fpos_t pos;
  bool finished = false;
  size_t c1PreviousValue = 0;
  while (!finished) {
    if (fscanf(pFile, "%zi %c %lli %c %lf", &c1, &separator, &c2, &separator, &c3) > 0) {
      fgetpos (pFile,&pos);
      fscanf (pFile, "%c", &separator);
      if (separator == ',') {
        // COM detected, read the end of line and do nothing else
        int res = fscanf (pFile, "%lf %c %hhi", &c4, &separator, &c5);
        vtAssertExpr(res == 3);
      } else {
        // Load detected, create the new element
        fsetpos (pFile,&pos);
        if (c1PreviousValue != c1) {
          c1PreviousValue = c1;
          StatsLBReader::user_specified_map_changed_.push_back(elements);
          elements.clear();
        }
        elements.emplace (c2, c3);
      }
    } else {
      finished = true;
    }
  }

  if (!elements.empty()) {
    StatsLBReader::user_specified_map_changed_.push_back(elements);
  }

  std::fclose(pFile);
}

/*static*/ void StatsLBReader::loadPhaseChangedMap() {
  auto const num_iters = StatsLBReader::user_specified_map_changed_.size() - 1;
  vt_print(lb, "StatsLBReader::loadPhaseChangedMap size : {}\n", num_iters);
  StatsLBReader::phase_changed_map_.vec_.resize(num_iters);

  for (size_t i = 0; i < num_iters; i++) {
    auto &unordered_elms = StatsLBReader::user_specified_map_changed_.at(i);
    auto elms = std::map<ElementIDType,TimeType>(unordered_elms.begin(),
                                                 unordered_elms.end());
    unordered_elms = StatsLBReader::user_specified_map_changed_.at(i + 1);
    auto elmsNext = std::map<ElementIDType,TimeType>(unordered_elms.begin(),
      unordered_elms.end());
    //
    StatsLBReader::phase_changed_map_.vec_[i] =
      !( (elmsNext.size() == elms.size()) and
      std::equal(elms.begin(), elms.end(), elmsNext.begin(),
        [](auto a, auto b) { return (a.first == b.first); }) );
  }
}

void StatsLBReader::doneReduce(VecPhaseMsg *msg) {
  vt_print(lb, "StatsLBReader::doneReduce with msg of size {}\n",
    msg->getConstVal().vec_.size());
}

void StatsLBReader::doReduce() {
  vt_print(lb, "StatsLBReader::doReduce {} \n",
    StatsLBReader::phase_changed_map_.vec_.size());

  auto cb = theCB()->makeBcast<StatsLBReader,VecPhaseMsg,&StatsLBReader::doneReduce>
    (StatsLBReader::proxy_);
  auto msg = makeMessage<VecPhaseMsg>(StatsLBReader::phase_changed_map_);
  StatsLBReader::proxy_.reduce<collective::OrOp<VectorDiffPhase>>(msg.get(),cb);
}

}}}} /* end namespace vt::vrt::collection::balance */
