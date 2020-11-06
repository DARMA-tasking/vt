/*
//@HEADER
// *****************************************************************************
//
//                                 composed_model.h
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

#if !defined INCLUDED_VRT_COLLECTION_BALANCE_COMPOSED_MODEL_H
#define INCLUDED_VRT_COLLECTION_BALANCE_COMPOSED_MODEL_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/model/load_model.h"

#include <checkpoint/checkpoint.h>

namespace vt { namespace vrt { namespace collection { namespace balance {

/**
 * \brief Utility class to support implementation of composable load
 * modeling components
 *
 * All model implementations meant to compose with arbitrary other
 * models should inherit from this class. It implements all methods by
 * calling the same method on the underlying model passed at
 * construction.
 */
class ComposedModel : public LoadModel
{
public:
  // \param[in] base must not be null
  explicit ComposedModel(std::shared_ptr<LoadModel> base) : base_(base) {}

  void setLoads(std::unordered_map<PhaseType, LoadMapType> const* proc_load,
                std::unordered_map<PhaseType, SubphaseLoadMapType> const* proc_subphase_load,
                std::unordered_map<PhaseType, CommMapType> const* proc_comm) override;

  void updateLoads(PhaseType last_completed_phase) override;

  TimeType getWork(ElementIDType object, PhaseOffset when) override;
  unsigned int getNumPastPhasesNeeded(unsigned int look_back) override;

  ObjectIterator begin() override;
  ObjectIterator end() override;

  int getNumObjects() override;
  unsigned int getNumCompletedPhases() override;
  int getNumSubphases() override;

private:
  std::shared_ptr<LoadModel> base_;
}; // class ComposedModel

}}}} // namespaces

#endif
