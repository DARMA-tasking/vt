/*
//@HEADER
// ************************************************************************
//
//                            invoke.cc
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

#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/vrt/collection/balance/lb_invoke/invoke.h"
#include "vt/vrt/collection/balance/lb_invoke/start_lb_msg.h"
#include "vt/vrt/collection/balance/read_lb.h"
#include "vt/vrt/collection/balance/lb_type.h"
#include "vt/vrt/collection/balance/hierarchicallb/hierlb.h"
#include "vt/vrt/collection/balance/greedylb/greedylb.h"
#include "vt/vrt/collection/balance/rotatelb/rotatelb.h"
#include "vt/vrt/collection/balance/gossiplb/gossiplb.h"
#include "vt/vrt/collection/messages/system_create.h"
#include "vt/vrt/collection/manager.fwd.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

/*static*/ PhaseType InvokeLB::cached_phase_ = no_lb_phase;
/*static*/ LBType InvokeLB::cached_lb_ = LBType::NoLB;

/*static*/ LBType InvokeLB::shouldInvoke(PhaseType phase, bool try_file) {
  debug_print(
    lb, node,
    "LBInvoke::shouldInvoke: phase={}, try_file={}, cached_phase_={}, lb={}\n",
    phase, try_file, cached_phase_, lb_names_[cached_lb_]
  );

  if (phase == cached_phase_) {
    return cached_lb_;
  } else {
    cached_phase_ = phase;
  }

  LBType the_lb = LBType::NoLB;

  if (ArgType::vt_lb_file and try_file) {
    auto const file_name = ArgType::vt_lb_file_name;
    ReadLBSpec::openFile(file_name);
    ReadLBSpec::readFile();
    bool const has_spec = ReadLBSpec::hasSpec();
    if (has_spec) {
      the_lb = ReadLBSpec::getLB(phase);
    }
  } else {
    vtAssert(ArgType::vt_lb_interval != 0, "LB Interval must not be 0");
    if (phase % ArgType::vt_lb_interval == 0) {
      for (auto&& elm : lb_names_) {
        if (elm.second == ArgType::vt_lb_name) {
          the_lb = elm.first;
          break;
        }
      }
    }
  }

  debug_print(
    lb, node,
    "LBInvoke::shouldInvoke: phase={}, return lb_={}\n",
    phase, lb_names_[the_lb]
  );

  cached_lb_ = the_lb;
  return the_lb;
}

/*static*/ void InvokeLB::startLBCollective(InvokeMsg* msg) {
  return startLBCollective(msg->phase_, msg->lb_);
}

/*static*/ void InvokeLB::startLBCollective(InvokeReduceMsg* msg) {
  return startLBCollective(msg->phase_, msg->lb_);
}

/*static*/ void InvokeLB::startLBCollective(PhaseType phase, LBType lb) {
  auto const& this_node = theContext()->getNode();

  if (this_node == 0 and not ArgType::vt_lb_quiet) {
    vt_print(
      lb,
      "InvokeLB::startLB: phase={}, balancer={}, name={}\n",
      phase,
      static_cast<typename std::underlying_type<LBType>::type>(lb),
      lb_names_[lb]
    );
  }

  auto msg = makeMessage<StartLBMsg>(phase);
  switch (lb) {
  case LBType::HierarchicalLB:
    lb::HierarchicalLB::hierLBHandler(msg.get());
    break;
  case LBType::GreedyLB:
    lb::GreedyLB::greedyLBHandler(msg.get());
    break;
  case LBType::RotateLB:
    lb::RotateLB::rotateLBHandler(msg.get());
    break;
  case LBType::GossipLB:
    lb::GossipLB::gossipLBHandler(msg.get());
    break;
  case LBType::NoLB:
    vtAssert(false, "LBType::NoLB is not a valid LB to startLBCollective");
    break;
  default:
    vtAssert(false, "A valid LB must be passed to startLBCollective");
    break;
  }

}

/*static*/ void InvokeLB::startLB(PhaseType phase, LBType lb) {
  //
  // The invocation should only happen on a single node. It can be any node, but
  // this function limits the invocation to node 0 to potentially catch when the
  // user invokes it multiple times incorrectly.
  //
  auto const& this_node = theContext()->getNode();
  vtAssert(this_node == 0, "InvokeLB::startLB should be invoked from node 0");

  auto msg = makeMessage<InvokeMsg>(phase,lb);
  theMsg()->broadcastMsg<InvokeMsg,InvokeLB::startLBCollective>(msg.get());
  InvokeLB::startLBCollective(msg.get());
}

/*static*/ void InvokeLB::releaseLB(PhaseType phase) {
  auto msg = makeMessage<InvokeMsg>(phase,LBType::NoLB);
  theMsg()->broadcastMsg<InvokeMsg,InvokeLB::releaseLBCollective>(msg.get());
  InvokeLB::releaseLBCollective(msg.get());
}

/*static*/ void InvokeLB::releaseLBCollective(PhaseType phase) {
  auto msg = makeMessage<CollectionPhaseMsg>();
  releaseLBPhase(msg.get());
}

/*static*/ void InvokeLB::releaseLBCollective(InvokeReduceMsg* msg) {
  return releaseLBCollective(msg->phase_);
}

/*static*/ void InvokeLB::releaseLBCollective(InvokeMsg* msg) {
  return releaseLBCollective(msg->phase_);
}

}}}} /* end namespace vt::vrt::collection::balance */
