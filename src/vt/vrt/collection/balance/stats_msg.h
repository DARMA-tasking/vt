/*
//@HEADER
// ************************************************************************
//
//                          stats_msg.h
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

#if !defined INCLUDED_VRT_COLLECTION_BALANCE_STATS_MSG_H
#define INCLUDED_VRT_COLLECTION_BALANCE_STATS_MSG_H

#include "vt/config.h"
#include "vt/collective/reduce/reduce.h"
#include "vt/messaging/message.h"
#include "vt/timing/timing.h"

#include <algorithm>

namespace vt { namespace vrt { namespace collection { namespace balance {

struct LoadData {
  LoadData() = default;
  LoadData(TimeType const& in_load_max, TimeType const& in_load_sum)
    : load_max_(in_load_max), load_sum_(in_load_sum)
  { }

  friend LoadData operator+(LoadData ld1, LoadData const& ld2) {
    auto const& sum_load = ld1.load_sum_ + ld2.load_sum_;
    auto const& max_load = std::max(ld1.load_max_,ld2.load_max_);
    ld1.load_sum_ = sum_load;
    ld1.load_max_ = max_load;
    return ld1;
  }

  TimeType loadMax() const { return load_max_; }
  TimeType loadSum() const { return load_sum_; }

  TimeType load_max_ = 0.0;
  TimeType load_sum_ = 0.0;
};

template <typename ColT>
struct LoadStatsMsg : CollectionMessage<ColT>, LoadData {
  LoadStatsMsg() = default;
  LoadStatsMsg(LoadData const& in_load_data, PhaseType const& phase)
    : LoadData(in_load_data), cur_phase_(phase)
  {}

  PhaseType getPhase() const { return cur_phase_; }

private:
  PhaseType cur_phase_ = fst_lb_phase;
};

struct ProcStatsMsg : collective::ReduceTMsg<LoadData> {
  ProcStatsMsg() = default;
  explicit ProcStatsMsg(TimeType const& in_total_load)
    : ReduceTMsg<LoadData>({in_total_load,in_total_load})
  { }
};

template <typename ColT>
struct StatsMsg : collective::ReduceTMsg<LoadData> {
  using ProxyType = typename ColT::CollectionProxyType;

  StatsMsg() = default;
  StatsMsg(
    PhaseType const& in_cur_phase, TimeType const& in_total_load,
    ProxyType const& in_proxy
  ) : ReduceTMsg<LoadData>({in_total_load,in_total_load}),
      proxy_(in_proxy), cur_phase_(in_cur_phase)
  { }

  ProxyType getProxy() const { return proxy_; }
  PhaseType getPhase() const { return cur_phase_; }
private:
  ProxyType proxy_ = {};
  PhaseType cur_phase_ = fst_lb_phase;
};

struct HierLBMsg : Message {
  HierLBMsg() = default;
  explicit HierLBMsg(PhaseType const& phase)
    : cur_phase_(phase)
  {}

  PhaseType getPhase() const { return cur_phase_; }

private:
  PhaseType cur_phase_ = fst_lb_phase;
};

using GreedyLBMsg = HierLBMsg;
using RotateLBMsg = HierLBMsg;

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_STATS_MSG_H*/
