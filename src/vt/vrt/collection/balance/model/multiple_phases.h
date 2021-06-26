/*
//@HEADER
// *****************************************************************************
//
//                              multiple_phases.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_MODEL_MULTIPLE_PHASES_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_MODEL_MULTIPLE_PHASES_H

#include "vt/vrt/collection/balance/model/composed_model.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

/**
 * \struct MultiplePhases
 *
 * \brief Predict an object's load as a sum over blocks of N future phases
 *
 * Expected to be most useful either when queried by an explicitly
 * subphase-aware vector-optimizing load balancer, or when queried by
 * a whole-phase scalar-optimizing load balancer with a Norm model
 * composed on top of this.
 *
 * Multiple phase blocked predictions will only be meaningfully
 * different from single phase predictions when composed on top of a
 * Predictor model that is not constant across future
 * phases. I.e. `LinearModel` rather than `NaivePersistence` or
 * `PersistenceMedianLastN`.
 */
struct MultiplePhases : ComposedModel {
  /**
   * \brief Constructor
   *
   * \param[in] base the base model
   *
   * \param[in] in_future_phase_block_size how many phases to predict
   * as each single queried phase
   */
  explicit MultiplePhases(
    std::shared_ptr<balance::LoadModel> base, int in_future_phase_block_size)
    : ComposedModel(base)
    , future_phase_block_size_(in_future_phase_block_size)
  { }

  TimeType getWork(ElementIDStruct object, PhaseOffset when) override;

private:
  int future_phase_block_size_ = 0;
};

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_MODEL_MULTIPLE_PHASES_H*/
