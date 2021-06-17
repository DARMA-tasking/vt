/*
//@HEADER
// *****************************************************************************
//
//                                 stats_pack.h
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

#if !defined INCLUDED_VT_RUNTIME_COMPONENT_METER_STATS_PACK_H
#define INCLUDED_VT_RUNTIME_COMPONENT_METER_STATS_PACK_H

#include "vt/config.h"

namespace vt { namespace runtime { namespace component { namespace meter {

struct Diagnostic;

struct DiagnosticMeter {};

/**
 * \internal \struct DiagnosticStatsPack
 *
 * \brief Pack of statistic-based diagnostics intended to back diagnostic types
 * where basic statistics should be applied
 */
template <typename T>
struct DiagnosticStatsPack : DiagnosticMeter {

  /**
   * \internal \brief Default constructor so diagnostics meters can be in
   * component classes and initialized later
   */
  DiagnosticStatsPack() = default;

  /**
   * \internal \brief Construct a new stats pack
   *
   * \param[in] in_sum the sum statistic
   * \param[in] in_avg the mean statistic
   * \param[in] in_max the max statistic
   * \param[in] in_min the min statistic
   */
  DiagnosticStatsPack(
    detail::DiagnosticValue<T>* in_sum,
    detail::DiagnosticValue<T>* in_avg,
    detail::DiagnosticValue<T>* in_max,
    detail::DiagnosticValue<T>* in_min
  ) : sum_(in_sum),
      avg_(in_avg),
      max_(in_max),
      min_(in_min)
  { }

  /**
   * \internal \brief Update the underlying stats pack
   *
   * \param[in] updated_val the updated value
   */
  void updateStats(T updated_val) {
    if (sum_ and avg_ and max_ and min_) {
      sum_->update(updated_val);
      avg_->update(updated_val);
      max_->update(updated_val);
      min_->update(updated_val);
    }
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | sum_
      | avg_
      | max_
      | min_;
  }

protected:
  detail::DiagnosticValue<T>* sum_ = nullptr; /**< Sum of all update values */
  detail::DiagnosticValue<T>* avg_ = nullptr; /**< Avg of all update values */
  detail::DiagnosticValue<T>* max_ = nullptr; /**< Max of all update values */
  detail::DiagnosticValue<T>* min_ = nullptr; /**< Min of all update values */
};

}}}} /* end namespace vt::runtime::component::meter */

#endif /*INCLUDED_VT_RUNTIME_COMPONENT_METER_STATS_PACK_H*/
