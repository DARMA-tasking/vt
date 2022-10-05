/*
//@HEADER
// *****************************************************************************
//
//                       weighted_communication_volume.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_MODEL_WEIGHTED_COMMUNICATION_VOLUME_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_MODEL_WEIGHTED_COMMUNICATION_VOLUME_H

#include "vt/vrt/collection/balance/model/composed_model.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

/**
 * \brief Models work as an affine combination of load and communication.
 */
class WeightedCommunicationVolume : public ComposedModel {
public:
  /**
   * \param[in] base underlying source of object work loads
   * \param[in] alpha load part coefficient
   * \param[in] beta communication part coefficient
   * \param[in] gamma unspecified constant cost
   */
  WeightedCommunicationVolume(
    std::shared_ptr<LoadModel> base,
    double alpha = 1.0, double beta = 0.0, double gamma = 0.0)
    : ComposedModel(base),
      alpha_(alpha),
      beta_(beta),
      gamma_(gamma) { }

  TimeType getModeledLoad(ElementIDStruct object, PhaseOffset when) const override;

private:
  double alpha_;
  double beta_;
  double gamma_;
};

}}}} // namespace vt::vrt::collection::balance

#endif
