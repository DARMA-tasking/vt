/*
//@HEADER
// *****************************************************************************
//
//                                 norm.cc
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


#include "vt/vrt/collection/balance/model/norm.h"
#include <cmath>

namespace vt { namespace vrt { namespace collection { namespace balance {

Norm::Norm(std::shared_ptr<balance::LoadModel> base, double power)
  : ComposedModel(base)
  , power_(power)
{
  vtAssert(not std::isnan(power), "Power must have a real value");
  vtAssert(power >= 0.0, "Reciprocal loads make no sense");
}

TimeType Norm::getWork(ElementIDStruct object, PhaseOffset offset)
{
  if (offset.subphase != PhaseOffset::WHOLE_PHASE)
    return ComposedModel::getWork(object, offset);

  if (std::isfinite(power_)) {
    double sum = 0.0;

    for (int i = 0; i < getNumSubphases(); ++i) {
      offset.subphase = i;
      auto t = ComposedModel::getWork(object, offset);
      sum += std::pow(t, power_);
    }

    return std::pow(sum, 1.0/power_);
  } else {
    // l-infinity implies a max norm
    double max = 0.0;

    for (int i = 0; i < getNumSubphases(); ++i) {
      offset.subphase = i;
      auto t = ComposedModel::getWork(object, offset);
      max = std::max(max, t);
    }

    return max;
  }
}


}}}}
