/*
//@HEADER
// *****************************************************************************
//
//                                  collect.h
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

#if !defined INCLUDED_LB_INSTRUMENTATION_CENTRALIZED_COLLECT_H
#define INCLUDED_LB_INSTRUMENTATION_CENTRALIZED_COLLECT_H

#include "vt/config.h"
#include "vt/lb/lb_types.h"
#include "vt/lb/lb_types_internal.h"
#include "vt/lb/instrumentation/database.h"
#include "vt/lb/instrumentation/centralized/collect_msg.h"

namespace vt { namespace lb { namespace instrumentation {

struct CentralCollect {
  static void startReduce(LBPhaseType const& phase);
  static void reduceCurrentPhase();
  static CollectMsg* collectStats(LBPhaseType const& phase);
  static void collectFinished(
    LBPhaseType const& phase, ProcContainerType const& entries
  );
  static LBPhaseType currentPhase();
  static void nextPhase();

  // Active message handlers
  static void centralizedCollect(CollectMsg* msg);
  static void combine(CollectMsg* msg1, CollectMsg* msg2);

private:
  static NodeType collect_root_;
  static LBPhaseType cur_lb_phase_;
};

}}} /* end namespace vt::lb::instrumentation */

#endif /*INCLUDED_LB_INSTRUMENTATION_CENTRALIZED_COLLECT_H*/
