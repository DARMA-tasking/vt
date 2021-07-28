/*
//@HEADER
// *****************************************************************************
//
//                              stats_replay.cc
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


#include "vt/vrt/collection/balance/model/stats_replay.h"
#include "vt/vrt/collection/balance/load_stats_replayer.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

StatsReplay::StatsReplay(
  std::shared_ptr<balance::LoadModel> base, ProxyType in_proxy
)
  : ComposedModel(base)
  , proxy_(in_proxy)
{
}

TimeType StatsReplay::getWork(ElementIDStruct object, PhaseOffset offset)
{
  auto const phase = getNumCompletedPhases() - 1;
  vt_debug_print(
    verbose, replay,
    "getWork {} phase={}\n",
    object.id, phase
  );

  vtAbortIf(
    offset.phases != PhaseOffset::NEXT_PHASE,
    "This driver only supports offset.phases == NEXT_PHASE"
  );
  vtAbortIf(
    offset.subphase != PhaseOffset::WHOLE_PHASE,
    "This driver only supports offset.subphase == WHOLE_PHASE"
  );

  auto index = vt::theLoadStatsReplayer()->getTypedIndexFromElm<Index2D>(object.id);

  auto elm_ptr = proxy_(index).tryGetLocalPtr();
  if (elm_ptr == nullptr) {
    vt_debug_print(
      verbose, replay,
      "getWork: could not find elm_id={} index={}\n",
      object.id, index
    );
    return 0; // FIXME: this BREAKS the O_l statistics post-migration!
  }
  vtAbortIf(elm_ptr == nullptr, "Must have element locally");
  auto load = elm_ptr->getLoad(phase);
  vt_debug_print(
    verbose, replay,
    "getWork: elm_id={} index={} has load={}\n",
    object.id, index, load
  );
  return load;
}

}}}}
