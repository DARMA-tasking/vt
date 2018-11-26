/*
//@HEADER
// ************************************************************************
//
//                          elm_stats.impl.h
//                                VT
//              Copyright (C) 2017 NTESS, LLC
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
#include "vt/vrt/collection/balance/read_lb.h"
#include "vt/vrt/collection/manager.h"
#include "vt/vrt/collection/balance/hierarchicallb/hierlb.h"
#include "vt/vrt/collection/balance/greedylb/greedylb.h"
#include "vt/vrt/collection/balance/rotatelb/rotatelb.h"
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
    vrt_coll, node,
    "ElementStats: syncNextPhase ({}) (idx={}): stats.getPhase()={}, "
    "msg->getPhase()={}\n",
    print_ptr(col), col->getIndex().x(), stats.getPhase(), msg->getPhase()
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

  auto phase_msg = makeSharedMessage<PhaseReduceMsg<ColT>>(cur_phase, proxy);

  using FunctorStatsType   = ComputeStats<ColT>;
  using FunctorStartLBType = StartLB<ColT>;

  if (lb_direct) {
    auto const before_ready = theCollection()->numReadyCollections();
    theCollection()->makeCollectionReady(untyped_proxy);
    auto const after_ready = theCollection()->numReadyCollections();
    auto const ready = theCollection()->readyNextPhase();
    debug_print(
      vrt_coll, node,
      "ElementStats: syncNextPhase: before_ready={}, after_ready={}, ready={}\n",
      before_ready, after_ready, ready
    );
    if (ready) {
      theCollection()->reduceMsg<
        ColT,
        PhaseReduceMsg<ColT>,
        PhaseReduceMsg<ColT>::template msgHandler<
          PhaseReduceMsg<ColT>,
          collective::PlusOp<collective::NoneType>,
          FunctorStartLBType
        >
      >(proxy, phase_msg, no_epoch, cur_phase);
    }
  } else {
    theCollection()->reduceMsg<
      ColT,
      PhaseReduceMsg<ColT>,
      PhaseReduceMsg<ColT>::template msgHandler<
        PhaseReduceMsg<ColT>,
        collective::PlusOp<collective::NoneType>,
        FunctorStatsType
      >
    >(proxy, phase_msg, no_epoch, cur_phase);
  }
}

template <typename ColT>
void ComputeStats<ColT>::operator()(PhaseReduceMsg<ColT>* msg) {
  auto const& proxy = msg->getProxy();
  auto const& cur_phase = msg->getPhase();
  auto phase_msg = makeSharedMessage<PhaseMsg<ColT>>(cur_phase,proxy);

  debug_print(
    vrt_coll, node,
    "ComputeStats: starting broadcast: phase={}\n", cur_phase
  );

  theCollection()->broadcastMsg<
    PhaseMsg<ColT>,
    ElementStats::computeStats<ColT>
  >(proxy, phase_msg, nullptr, false);
}

template <typename ColT>
/*static*/ void ElementStats::computeStats(PhaseMsg<ColT>* msg, ColT* col) {
  using MsgType = StatsMsg<ColT>;
  auto& stats = col->stats_;
  auto const& cur_phase = msg->getPhase();
  auto const& total_load = stats.getLoad(cur_phase);
  auto const& proxy = col->getCollectionProxy();
  auto stats_msg = makeSharedMessage<MsgType>(cur_phase, total_load, proxy);

  debug_print(
    vrt_coll, node,
    "ComputeStats ({}) (idx={}) (reduce): phase={}, load={}\n",
    print_ptr(col), col->getIndex().x(), cur_phase, total_load
  );

  theCollection()->reduceMsg<
    ColT,
    MsgType,
    MsgType::template msgHandler<
      MsgType, collective::PlusOp<LoadData>, CollectedStats<ColT>
    >
  >(proxy, stats_msg, no_epoch, cur_phase);
}

template <typename ColT>
/*static*/ void ElementStats::statsIn(LoadStatsMsg<ColT>* msg, ColT* col) {
  debug_print(
    vrt_coll, node,
    "ElementsStats::statsIn: max={}, sum={}, avg={}\n",
    msg->load_max_, msg->load_sum_, msg->load_sum_/8
  );

  auto& stats = col->stats_;
  auto const& cur_phase = msg->getPhase();
  auto const& total_load = stats.getLoad(cur_phase);
  auto const& proxy = col->getCollectionProxy();
  auto const& idx = col->getIndex();
  auto const& elm_proxy = proxy[idx];
  ProcStats::addProcStats<ColT>(elm_proxy, col, msg->getPhase(), total_load);

  auto phase_msg = makeSharedMessage<PhaseReduceMsg<ColT>>(cur_phase,proxy);
  theCollection()->reduceMsg<
    ColT,
    PhaseReduceMsg<ColT>,
    PhaseReduceMsg<ColT>::template msgHandler<
      PhaseReduceMsg<ColT>,
      collective::PlusOp<collective::NoneType>,
      StartLB<ColT>
    >
  >(proxy, phase_msg, no_epoch, cur_phase);
}

template <typename ColT>
void CollectedStats<ColT>::operator()(StatsMsg<ColT>* msg) {
  auto load_msg = makeSharedMessage<LoadStatsMsg<ColT>>(
    msg->getConstVal(), msg->getPhase()
  );

  debug_print(
    vrt_coll, node,
    "CollectedStats (broadcast): phase={}\n",
    msg->getPhase()
  );

  theCollection()->broadcastMsg<
    LoadStatsMsg<ColT>, ElementStats::statsIn<ColT>
  >(msg->getProxy(), load_msg, nullptr, false);
}

template <typename ColT>
void StartLB<ColT>::operator()(PhaseReduceMsg<ColT>* msg) {
  auto const& this_node = theContext()->getNode();
  auto const& phase = msg->getPhase();
  auto the_lb = theContext()->getLB();

  using namespace balance;
  ReadLBSpec::openFile();
  ReadLBSpec::readFile();

  bool const has_spec = ReadLBSpec::hasSpec();
  if (has_spec) {
    the_lb = ReadLBSpec::getLB(phase);
  }

  if (this_node == 0) {
    ::fmt::print(
      "VT: {}: StartLB: phase={}, balancer={}, name={}\n",
      this_node,
      msg->getPhase(),
      static_cast<typename std::underlying_type<LBType>::type>(the_lb),
      lb_names_<>[the_lb]
    );
  }

  switch (the_lb) {
  case LBType::HierarchicalLB:
  {
    auto nmsg = makeSharedMessage<HierLBMsg>(msg->getPhase());
    theMsg()->broadcastMsg<HierLBMsg,lb::HierarchicalLB::hierLBHandler>(nmsg);
    auto nmsg_root = makeSharedMessage<HierLBMsg>(msg->getPhase());
    lb::HierarchicalLB::hierLBHandler(nmsg_root);
  }
  break;
  case LBType::GreedyLB:
  {
    auto nmsg = makeSharedMessage<GreedyLBMsg>(msg->getPhase());
    theMsg()->broadcastMsg<GreedyLBMsg,lb::GreedyLB::greedyLBHandler>(nmsg);
    auto nmsg_root = makeSharedMessage<GreedyLBMsg>(msg->getPhase());
    lb::GreedyLB::greedyLBHandler(nmsg_root);
  }
  break;
  case LBType::RotateLB:
  {
    auto nmsg = makeSharedMessage<RotateLBMsg>(msg->getPhase());
    theMsg()->broadcastMsg<RotateLBMsg,lb::RotateLB::rotateLBHandler>(nmsg);
    auto nmsg_root = makeSharedMessage<RotateLBMsg>(msg->getPhase());
    lb::RotateLB::rotateLBHandler(nmsg_root);
  }
  break;
  default:
    theCollection()->releaseLBContinuation();
    break;
  }
}

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_ELM_STATS_IMPL_H*/
