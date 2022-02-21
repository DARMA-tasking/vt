/*
//@HEADER
// *****************************************************************************
//
//                            stats_replay.impl.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_MODEL_STATS_REPLAY_IMPL_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_MODEL_STATS_REPLAY_IMPL_H

#include "vt/vrt/collection/balance/model/stats_replay.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

template <typename CollectionIndexType>
StatsReplay<CollectionIndexType>::StatsReplay(
  std::shared_ptr<balance::LoadModel> base
)
  : ComposedModel(base)
{
}

template <typename CollectionIndexType>
TimeType StatsReplay<CollectionIndexType>::getWork(
  ElementIDStruct object, PhaseOffset offset
) {
  vtAbortIf(
    offset.phases != PhaseOffset::NEXT_PHASE,
    "This load model only supports offset.phases == NEXT_PHASE"
  );

  auto const phase = getNumCompletedPhases() - 1;
  auto index = mapper_.getIndexFromElm(object.id);
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
  auto load_summary = elm_ptr->getLoad(phase);
  if (
    offset.subphase != PhaseOffset::WHOLE_PHASE and
    load_summary.subphase_loads.size() == 0
  ) {
    // our input statistics do not contain subphases; attribute the whole phase
    // load to the first subphase and nothing to any remaining subphases
    auto assumed_subphase_load = load_summary.whole_phase_load;
    if (offset.subphase > 0) {
      assumed_subphase_load = 0;
    }
    vt_debug_print(
      verbose, replay,
      "getWork: elm_id={} index={} subphase={} has assumed load={}\n",
      object.id, index, offset.subphase, assumed_subphase_load
    );
    return assumed_subphase_load;
  } else {
    auto load = load_summary.get(offset);
    vt_debug_print(
      verbose, replay,
      "getWork: elm_id={} index={} subphase={} has load={}\n",
      object.id, index, offset.subphase, load
    );
    return load;
  }
}

}}}} // end namespace

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_MODEL_STATS_REPLAY_IMPL_H*/
