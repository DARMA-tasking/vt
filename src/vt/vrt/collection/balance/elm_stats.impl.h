/*
//@HEADER
// ************************************************************************
//
//                          elm_stats.impl.h
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

#if !defined INCLUDED_VRT_COLLECTION_BALANCE_ELM_STATS_IMPL_H
#define INCLUDED_VRT_COLLECTION_BALANCE_ELM_STATS_IMPL_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/elm_stats.h"
#include "vt/vrt/collection/balance/phase_msg.h"
#include "vt/vrt/collection/balance/stats_msg.h"
#include "vt/vrt/collection/balance/proc_stats.h"
#include "vt/vrt/collection/balance/lb_type.h"
#include "vt/vrt/collection/manager.h"
#include "vt/vrt/collection/balance/lb_invoke/invoke.h"
#include "vt/timing/timing.h"

#include <cassert>
#include <type_traits>

namespace vt { namespace vrt { namespace collection { namespace balance {

template <typename Serializer>
void ElementStats::serialize(Serializer& s) {
  s | cur_time_started_;
  s | cur_time_;
  s | cur_phase_;
  s | phase_timings_;
}

template <typename ColT>
/*static*/ void ElementStats::syncNextPhase(PhaseMsg<ColT>* msg, ColT* col) {
  auto& stats = col->stats_;

  debug_print(
    lb, node,
    "ElementStats: syncNextPhase ({}) (idx={}): stats.getPhase()={}, "
    "msg->getPhase()={}\n",
    print_ptr(col), col->getIndex(), stats.getPhase(), msg->getPhase()
  );

  vtAssert(stats.getPhase() == msg->getPhase(), "Phases must match");
  stats.updatePhase(1);

  auto const& cur_phase = msg->getPhase();
  auto const& proxy = col->getCollectionProxy();
  auto const& untyped_proxy = col->getProxy();
  auto const& total_load = stats.getLoad(cur_phase);
  auto const& idx = col->getIndex();
  auto const& elm_proxy = proxy[idx];
  ProcStats::addProcStats<ColT>(elm_proxy, col, cur_phase, total_load);

  auto const before_ready = theCollection()->numReadyCollections();
  theCollection()->makeCollectionReady(untyped_proxy);
  auto const after_ready = theCollection()->numReadyCollections();
  auto const ready = theCollection()->readyNextPhase();

  debug_print(
    lb, node,
    "ElementStats: syncNextPhase: before_ready={}, after_ready={}, ready={}\n",
    before_ready, after_ready, ready
  );

  if (ready) {
    using MsgType = InvokeReduceMsg;
    auto const do_sync = msg->doSync();
    auto const lb = InvokeLB::shouldInvoke(cur_phase);
    Callback<MsgType> cb_;
    if (lb != LBType::NoLB) {
      cb_ = theCB()->makeBcast<MsgType,InvokeLB::startLBCollective>();
    } else if (do_sync) {
      cb_ = theCB()->makeBcast<MsgType,InvokeLB::releaseLBCollective>();
    }
    if (lb != LBType::NoLB or do_sync) {
      auto msg = makeMessage<MsgType>(cur_phase,lb);
      proxy.reduce(msg.get(),cb_);
    } else {
      theCollection()->elmFinishedLB(elm_proxy,cur_phase);
    }
  }
}

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_ELM_STATS_IMPL_H*/
