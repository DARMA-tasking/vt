/*
//@HEADER
// *****************************************************************************
//
//                                 comm_overhead.h
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

#if !defined INCLUDED_VRT_COLLECTION_BALANCE_COMM_OVERHEAD_H
#define INCLUDED_VRT_COLLECTION_BALANCE_COMM_OVERHEAD_H

#include "vt/vrt/collection/balance/model/composed_model.h"
#include <unordered_map>

namespace vt { namespace vrt { namespace collection { namespace balance {

/**
 * \brief Add some implied, unaccounted work time for communication activity
 */
struct CommOverhead : public ComposedModel {
  /**
   * \brief Constructor
   *
   * \param[in] base: the underlying source of object work loads
   * \param[in] in_per_msg_weight weight to add per message received
   * \param[in] in_per_byte_weight weight to add per byte received
   */
  explicit CommOverhead(
    std::shared_ptr<balance::LoadModel> base, TimeType in_per_msg_weight,
    TimeType in_per_byte_weight
  );

  checkpoint_virtual_serialize_derived_from(ComposedModel)

  void setLoads(std::unordered_map<PhaseType, LoadMapType> const* proc_load,
                std::unordered_map<PhaseType, SubphaseLoadMapType> const* proc_subphase_load,
                std::unordered_map<PhaseType, CommMapType> const* proc_comm) override;

  TimeType getWork(ElementIDType object, PhaseOffset when) override;

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | proc_comm_
      | per_msg_weight_
      | per_byte_weight_;
  }

private:
  std::unordered_map<PhaseType, CommMapType> const* proc_comm_; /**< Underlying comm data */
  TimeType per_msg_weight_ = 0.001;           /**< Cost per message */
  TimeType per_byte_weight_ = 0.000001;       /**< Cost per bytes */
}; // class CommOverhead

}}}} // end namespace

#endif
