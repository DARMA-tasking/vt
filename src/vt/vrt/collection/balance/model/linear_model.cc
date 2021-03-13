/*
//@HEADER
// *****************************************************************************
//
//                               linear_model.cc
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

#include "vt/vrt/collection/balance/model/linear_model.h"
#include "vt/utils/stats/linear_regression.h"

#include <algorithm>

namespace vt { namespace vrt { namespace collection { namespace balance {

TimeType LinearModel::getWork(ElementIDStruct object, PhaseOffset when) {
  using util::stats::LinearRegression;

  // Retrospective queries don't call for a prediction
  if (when.phases < 0)
    return ComposedModel::getWork(object, when);

  std::vector<double> x;
  std::vector<double> y;

  PhaseOffset past_phase{when};

  unsigned int phases = std::min(past_len_, getNumCompletedPhases());
  // Number values on X-axis based on a PhaseOffset
  for (int i = -1 * static_cast<int>(phases); i < 0; i++) {
    x.emplace_back(i);
    past_phase.phases = i;
    y.emplace_back(ComposedModel::getWork(object, past_phase));
  }

  // should we re-create this every time?
  LinearRegression regression{x, y};
  return regression.predict(when.phases);
}

unsigned int LinearModel::getNumPastPhasesNeeded(unsigned int look_back)
{
  return ComposedModel::getNumPastPhasesNeeded(std::max(past_len_, look_back));
}

}}}} /* end namespace vt::vrt::collection::balance */
