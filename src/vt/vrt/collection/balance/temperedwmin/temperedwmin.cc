/*
//@HEADER
// *****************************************************************************
//
//                               temperedwmin.cc
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

#include "vt/vrt/collection/balance/temperedwmin/temperedwmin.h"

#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/vrt/collection/balance/model/load_model.h"

namespace vt { namespace vrt { namespace collection { namespace lb {

void TemperedWMin::init(objgroup::proxy::Proxy<TemperedWMin> in_proxy) {
  auto proxy_bits = in_proxy.getProxy();
  auto proxy = objgroup::proxy::Proxy<TemperedLB>(proxy_bits);
  auto strat = proxy.get();
  strat->init(proxy);
}

/*static*/ std::unordered_map<std::string, std::string>
TemperedWMin::getInputKeysWithHelp() {
  auto map = TemperedLB::getInputKeysWithHelp();
  map["alpha"] =
    R"(
Values: <double>
Default: 1.0
Description:
  Load part coefficient in affine combination of load and communication.
)";
  map["beta"] =
    R"(
Values: <double>
Default: 0.0
Description:
  Communication part coefficient in affine combination of load and communication.
)";
  map["gamma"] =
    R"(
Values: <double>
Default: 0.0
Description:
  Unspecified constant cost.
)";
  return map;
}

void TemperedWMin::inputParams(balance::SpecEntry* spec) {
  TemperedLB::inputParams(spec);

  alpha_         = spec->getOrDefault<double>("alpha", alpha_);
  beta_          = spec->getOrDefault<double>("beta", beta_);
  gamma_         = spec->getOrDefault<double>("gamma", gamma_);
}

TimeType TemperedWMin::getTotalWork(const elm::ElementIDStruct& obj) {
  balance::PhaseOffset when =
      {balance::PhaseOffset::NEXT_PHASE, balance::PhaseOffset::WHOLE_PHASE};

  return alpha_ * load_model_->getLoadMetric(obj, when) +
    beta_ * load_model_->getComm(obj, when) + gamma_;
}

}}}} // namespace vt::vrt::collection::lb
