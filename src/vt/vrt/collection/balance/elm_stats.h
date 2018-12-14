/*
//@HEADER
// ************************************************************************
//
//                          elm_stats.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_VRT_COLLECTION_BALANCE_ELM_STATS_H
#define INCLUDED_VRT_COLLECTION_BALANCE_ELM_STATS_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/elm_stats.fwd.h"
#include "vt/vrt/collection/balance/phase_msg.h"
#include "vt/vrt/collection/balance/stats_msg.h"
#include "vt/timing/timing.h"
#include "vt/vrt/collection/types/migratable.fwd.h"
#include "vt/configs/arguments/args.h"

#include <cstdint>
#include <vector>

namespace vt { namespace vrt { namespace collection { namespace balance {

static constexpr bool const lb_direct = true;

struct ElementStats {
  using PhaseType = uint64_t;
  using ArgType   = vt::arguments::ArgConfig;

  ElementStats() = default;
  ElementStats(ElementStats const&) = default;
  ElementStats(ElementStats&&) = default;

  void startTime();
  void stopTime();
  void addTime(TimeType const& time);
  void setModelWeight(TimeType const& time);
  void updatePhase(PhaseType const& inc = 1);
  PhaseType getPhase() const;
  TimeType getLoad(PhaseType const& phase) const;

  template <typename Serializer>
  void serialize(Serializer& s);

public:
  template <typename ColT>
  static void syncNextPhase(PhaseMsg<ColT>* msg, ColT* col);

  template <typename ColT>
  static void computeStats(PhaseMsg<ColT>* msg, ColT* col);

  template <typename ColT>
  static void statsIn(LoadStatsMsg<ColT>* msg, ColT* col);

  template <typename ColT>
  friend struct collection::Migratable;

protected:
  bool cur_time_started_ = false;
  TimeType cur_time_ = 0.0;
  PhaseType cur_phase_ = fst_lb_phase;
  std::vector<TimeType> phase_timings_ = {};
};

template <typename ColT>
struct ComputeStats {
  void operator()(PhaseReduceMsg<ColT>* msg);
};

template <typename ColT>
struct CollectedStats {
  void operator()(StatsMsg<ColT>* msg);
};

template <typename ColT>
struct StartLB {
  using ArgType = vt::arguments::ArgConfig;
  void operator()(PhaseReduceMsg<ColT>* msg);
};

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_ELM_STATS_H*/
