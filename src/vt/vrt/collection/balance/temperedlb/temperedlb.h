/*
//@HEADER
// *****************************************************************************
//
//                                 temperedlb.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_TEMPEREDLB_TEMPEREDLB_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_TEMPEREDLB_TEMPEREDLB_H

#include "vt/vrt/collection/balance/baselb/baselb.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace vt { namespace vrt { namespace collection { namespace lb {

/**
 * \brief VT adapter for TemperedLB from the DARMA-tasking/LB repository.
 *
 * The algorithm and its communication backend live in the external LB and
 * comm repositories. This class only translates VT's load model into the
 * runtime-independent LB model and translates the resulting mapping back into
 * VT migrations.
 */
struct TemperedLB : BaseLB {
  struct Mapping {
    std::vector<std::pair<uint64_t, NodeType>> tasks;

    friend Mapping operator+(Mapping lhs, Mapping const& rhs) {
      lhs.tasks.insert(lhs.tasks.end(), rhs.tasks.begin(), rhs.tasks.end());
      return lhs;
    }

    template <typename SerializerT>
    void serialize(SerializerT& s) {
      s | tasks;
    }
  };

  TemperedLB() = default;
  TemperedLB(TemperedLB const&) = delete;
  ~TemperedLB() override = default;

  template <typename ProxyT>
  void init(ProxyT const& proxy) {
    proxy_ = objgroup::proxy::Proxy<TemperedLB>{proxy.getProxy()};
  }

  void inputParams(balance::ConfigEntry* config) override;
  void runLB(LoadType total_load) override;
  void collectMapping(Mapping mapping) { global_mapping_ = std::move(mapping); }

  static std::unordered_map<std::string, std::string> getInputKeysWithHelp();

protected:
  void setWorkModel(double alpha, double beta, double gamma, double delta = 0.0) {
    alpha_ = alpha;
    beta_ = beta;
    gamma_ = gamma;
    delta_ = delta;
  }

private:
  int32_t fanout_ = 2;
  int32_t rounds_ = 1;
  int32_t num_iters_ = 10;
  int32_t num_trials_ = 1;
  int32_t seed_ = 29;

  bool deterministic_ = true;
  double alpha_ = 1.0;
  double beta_ = 0.0;
  double gamma_ = 0.0;
  double delta_ = 0.0;
  double memory_threshold_ = 0.0;
  double converge_tolerance_ = 0.01;

  std::string criterion_ = "ModifiedGrapevine";
  std::string ordering_ = "ElmID";
  std::string cmf_ = "NormByMaxExcludeIneligible";

  objgroup::proxy::Proxy<TemperedLB> proxy_ = {};
  Mapping global_mapping_ = {};
};

}}}} /* end namespace vt::vrt::collection::lb */

#endif /* INCLUDED_VT_VRT_COLLECTION_BALANCE_TEMPEREDLB_TEMPEREDLB_H */
