/*
//@HEADER
// *****************************************************************************
//
//                              select_subphases.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_MODEL_SELECT_SUBPHASES_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_MODEL_SELECT_SUBPHASES_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/model/composed_model.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

/**
 * \brief A load model to direct consideration to load data relating
 * to a specific set of subphases, rather than the entire set.
 *
 * This may be useful, for example, to select only subphases that are
 * substantially imbalanced, or to exclude subphases in which loads
 * are unpredictable.
 */
class SelectSubphases : public ComposedModel
{
public:
  /**
   * \brief Constructor taking an enumeration of the relevant subphases
   *
   * \param[in] base: The source of underlying load numbers to return; must not be null
   * \param[in] subphases: The set of subphases to expose to callers
   */
  SelectSubphases(std::shared_ptr<LoadModel> base, std::vector<unsigned int> subphases);

  TimeType getWork(ElementIDStruct object, PhaseOffset when) override;
  int getNumSubphases() override;

  std::vector<unsigned int> subphases_;
}; // class SelectSubphases

}}}} // namespaces

#endif
