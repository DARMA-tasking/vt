/*
//@HEADER
// *****************************************************************************
//
//                               elm_stats.impl.h
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

#if !defined INCLUDED_VRT_COLLECTION_BALANCE_ELM_STATS_IMPL_H
#define INCLUDED_VRT_COLLECTION_BALANCE_ELM_STATS_IMPL_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/elm_stats.h"
#include "vt/vrt/collection/balance/phase_msg.h"
#include "vt/vrt/collection/balance/stats_msg.h"
#include "vt/vrt/collection/balance/lb_type.h"
#include "vt/vrt/collection/manager.h"
#include "vt/vrt/collection/balance/lb_invoke/lb_manager.h"
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
  s | comm_;
  s | cur_subphase_;
  s | subphase_timings_;
}

template <typename ColT>
/*static*/ void ElementStats::syncNextPhase(PhaseMsg<ColT>* msg, ColT* col) {
  auto& stats = col->stats_;

  vt_debug_print(
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
  auto const& total_load = stats.getLoad(cur_phase, getFocusedSubPhase(untyped_proxy));
  auto const& subphase_loads = stats.subphase_timings_.at(cur_phase);
  auto const& comm = stats.getComm(cur_phase);
  auto const& subphase_comm = stats.getSubphaseComm(cur_phase);
  auto const& idx = col->getIndex();
  auto const& elm_proxy = proxy[idx];

  theNodeStats()->addNodeStats(col, cur_phase, total_load, subphase_loads, comm, subphase_comm);

  auto const before_ready = theCollection()->numReadyCollections();
  theCollection()->makeCollectionReady(untyped_proxy);
  auto const after_ready = theCollection()->numReadyCollections();
  auto const ready = theCollection()->readyNextPhase();

  vt_debug_print(
    lb, node,
    "ElementStats: syncNextPhase: before_ready={}, after_ready={}, ready={}\n",
    before_ready, after_ready, ready
  );

  auto lb_man = theLBManager()->getProxy();

  auto const single_node = theContext()->getNumNodes() == 1;
  auto const lb = lb_man.get()->decideLBToRun(cur_phase);
  bool const must_run_lb = lb != LBType::NoLB and not single_node;
  auto const num_collections = theCollection()->numCollections<>();
  auto const do_sync = msg->doSync();
  auto nmsg = makeMessage<InvokeMsg>(cur_phase,lb,msg->manual(),num_collections);

  if (must_run_lb) {
    auto cb = theCB()->makeBcast<LBManager,InvokeMsg,&LBManager::sysLB>(lb_man);
    proxy.reduce(nmsg.get(),cb);
  } else {

    // Preemptively release the element directly, doing cleanup later after a
    // collection reduction. This allows work to start early while still
    // releasing the node-level LB continuations needed for cleanup
    if (lb == LBType::NoLB and not do_sync) {
      theCollection()->elmFinishedLB(elm_proxy,cur_phase);
    }

    auto cb = theCB()->makeBcast<LBManager,InvokeMsg,&LBManager::sysReleaseLB>(lb_man);
    proxy.reduce(nmsg.get(),cb);
  }
}

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_ELM_STATS_IMPL_H*/
