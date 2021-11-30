/*
//@HEADER
// *****************************************************************************
//
//                               load_sampler.cc
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

#include "vt/vrt/collection/balance/baselb/load_sampler.h"

namespace vt { namespace vrt { namespace collection { namespace lb {

void LoadSamplerBaseLB::buildHistogram() {
  for (auto it = load_model_->begin(); it != load_model_->end(); ++it) {
    auto obj = *it;
    auto load = load_model_->getWork(
      obj, {balance::PhaseOffset::NEXT_PHASE, balance::PhaseOffset::WHOLE_PHASE}
    );
    auto const& load_milli = loadMilli(load);
    auto const& bin = histogramSample(load_milli);
    obj_sample[bin].push_back(obj);

    vt_debug_print(
      verbose, lb,
      "\t buildHistogram: obj={}, home={}, load={}, "
      "load_milli={}, bin={}\n",
      obj.id, obj.home_node, load, load_milli, bin
    );
  }
}

LoadSamplerBaseLB::ObjBinType
LoadSamplerBaseLB::histogramSample(LoadType const& load) const {
  auto const bin_size = getBinSize();
  ObjBinType const bin =
    ((static_cast<int32_t>(load)) / bin_size * bin_size)
    + bin_size;
  return bin;
}

}}}} /* end namespace vt::vrt::collection::lb */
