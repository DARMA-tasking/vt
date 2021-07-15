/*
//@HEADER
// *****************************************************************************
//
//                          diagnostic_erased_value.h
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

#if !defined INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_ERASED_VALUE_H
#define INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_ERASED_VALUE_H

#include "vt/runtime/component/diagnostic_types.h"
#include "vt/utils/adt/union.h"
#include "vt/utils/adt/histogram_approx.h"

#include <string>

namespace vt { namespace runtime { namespace component {

/**
 * \struct DiagnosticErasedValue
 *
 * \brief Typed-erased, diagnostic values as strings for outputting to user
 * after being formatted
 */
struct DiagnosticErasedValue {
  /// These are the set of valid diagnostic value types after being erased from
  /// \c DiagnosticValue<T> get turned into this union for saving the value.
  using UnionValueType = vt::adt::SafeUnion<
    double, float,
    int8_t,  int16_t,  int32_t,  int64_t,
    uint8_t, uint16_t, uint32_t, uint64_t
  >;

  // The trio (min, max, sum) save the actual type with the value to print it
  // correctly
  UnionValueType min_; /// min over all nodes
  UnionValueType max_; /// max over all nodes
  UnionValueType sum_; /// sum total over all nodes

  // The remainder (avg, std) are always converted to doubles since they are
  // stats derived from the underlying value type
  double avg_ = 0.; /// mean over all nodes
  double std_ = 0.; /// standard deviation over all nodes

  /// The updater type (SUM, MIN, MAX, REPLACE, AVG, etc.)
  DiagnosticUpdate update_ = DiagnosticUpdate::Sum;

  /// The value's unit type: Bytes, Seconds, Units, etc.
  DiagnosticUnit unit_ = DiagnosticUnit::Units;

  /// Whether the value turned out to be valid (i.e., if a MIN updater ends up
  /// with the sentinel value \c std::numeric_limits<T>::min() after reduction,
  /// it was never updated and \c is_valid_value_ will be false)
  bool is_valid_value_ = false;

  // Histogram of values across all nodes
  adt::HistogramApprox<double, int64_t> hist_;
};

}}} /* end namespace vt::runtime::component */

#endif /*INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_ERASED_VALUE_H*/
