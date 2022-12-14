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

#include "vt/configs/error/config_assert.h"
#include "vt/vrt/collection/balance/lb_invoke/lb_manager.h"
#include "vt/vrt/collection/balance/model/load_model.h"
#include "vt/vrt/collection/balance/model/weighted_communication_volume.h"

namespace vt { namespace vrt { namespace collection { namespace lb {

TemperedWMin::~TemperedWMin() {
  setStrategySpecificModel(nullptr);
}

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

void TemperedWMin::inputParams(balance::ConfigEntry* config) {
  TemperedLB::inputParams(config);

  alpha_         = config->getOrDefault<double>("alpha", alpha_);
  beta_          = config->getOrDefault<double>("beta", beta_);
  gamma_         = config->getOrDefault<double>("gamma", gamma_);

  vt_debug_print(
    normal, temperedwmin,
    "TemperedWMin::inputParams: alpha={}, beta={}, gamma={}\n",
    alpha_, beta_, gamma_
  );

  total_work_model_ = std::make_shared<balance::WeightedCommunicationVolume>(
    theLBManager()->getLoadModel(), alpha_, beta_, gamma_
  );
  setStrategySpecificModel(total_work_model_);

  // for later assertion only
  load_model_ptr = theLBManager()->getLoadModel().get();
}

TimeType TemperedWMin::getModeledValue(const elm::ElementIDStruct& obj) {
  vtAssert(
    theLBManager()->getLoadModel().get() == load_model_ptr,
    "Load model must not change"
  );
  balance::PhaseOffset when =
      {balance::PhaseOffset::NEXT_PHASE, balance::PhaseOffset::WHOLE_PHASE};

  return total_work_model_->getModeledLoad(obj, when);
}

}}}} // namespace vt::vrt::collection::lb
