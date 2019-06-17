/*
//@HEADER
// ************************************************************************
//
//                          rotatelb.cc
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
#include "vt/vrt/collection/balance/rotatelb/rotatelb.h"
#include "vt/vrt/collection/manager.h"

#include <memory>

namespace vt { namespace vrt { namespace collection { namespace lb {

void RotateLB::procDataIn(ElementLoadType const& data_in) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();
  auto const next_node = this_node + 1 > num_nodes-1 ? 0 : this_node + 1;
  if (this_node == 0) {
    vt_print(
      lb,
      "RotateLB: procDataIn: stats size={}, next_node={}\n",
      data_in.size(), next_node
    );
    fflush(stdout);
  }
  debug_print(
    lb, node,
    "RotateLB::procDataIn: size={}, next_node={}\n",
    data_in.size(), next_node
  );
  auto const epoch = theTerm()->makeEpochCollective();
  theMsg()->pushEpoch(epoch);
  theTerm()->addActionEpoch(epoch,[this]{ this->finishedMigrate(); });
  for (auto&& stat : data_in) {
    auto const& obj = stat.first;
    auto const& load = stat.second;
    debug_print(
      lb, node,
      "\t RotateLB::procDataIn: obj={}, load={}\n",
      obj, load
    );
    auto iter = balance::ProcStats::proc_migrate_.find(obj);
    vtAssert(iter != balance::ProcStats::proc_migrate_.end(), "Must exist");
    transfer_count++;
    iter->second(next_node);
  }
  theTerm()->finishedEpoch(epoch);
  theMsg()->popEpoch();
}

void RotateLB::finishedMigrate() {
  debug_print(
    lb, node,
    "RotateLB::finishedMigrate: transfer_count={}\n",
    transfer_count
  );
  balance::ProcStats::proc_migrate_.clear();
  balance::ProcStats::proc_data_.clear();
  balance::ProcStats::next_elm_ = 1;
  theCollection()->releaseLBContinuation();
}

/*static*/ void RotateLB::rotateLBHandler(balance::RotateLBMsg* msg) {
  auto const& phase = msg->getPhase();
  RotateLB::rotate_lb_inst = std::make_unique<RotateLB>();
  vtAssertExpr(balance::ProcStats::proc_data_.size() >= phase);
  debug_print(
    lb, node,
    "\t RotateLB::rotateLBHandler: phase={}\n", phase
  );
  RotateLB::rotate_lb_inst->procDataIn(balance::ProcStats::proc_data_[phase]);
}

/*static*/ std::unique_ptr<RotateLB> RotateLB::rotate_lb_inst;

}}}} /* end namespace vt::vrt::collection::lb */

