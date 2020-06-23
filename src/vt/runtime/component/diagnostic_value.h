/*
//@HEADER
// *****************************************************************************
//
//                              diagnostic_value.h
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

#if !defined INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_VALUE_H
#define INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_VALUE_H

#include "vt/runtime/component/diagnostic_types.h"
#include "vt/runtime/component/diagnostic_string.h"
#include "vt/runtime/component/diagnostic_value_base.h"

#include <string>
#include <cmath>

namespace vt { namespace runtime { namespace component { namespace detail {

/**
 * \struct DiagnosticValueWrapper
 *
 * \brief Wrapper for the actual diagnostic value, required so reductions do not
 * have to operate over the virtual \c DiagnosticBase hierarchy.
 */
template <typename T>
struct DiagnosticValueWrapper {
  DiagnosticValueWrapper() = default;

  /**
   * \internal \brief Construct a regular value with a initial value
   *
   * \param[in] in_value the initial value
   */
  explicit DiagnosticValueWrapper(T in_value)
    : value_(in_value)
  { }

  /**
   * \struct ReduceTag
   *
   * \internal \brief Tag for creating a value wrapper that will participate in
   * the reduction
   */
  struct ReduceTag {};

  /**
   * \internal \brief Construct a wrapper value for the reduction
   *
   * \param[in] ReduceTag reduce tag to distinguish it
   * \param[in] in_value the starting value for the reduction
   */
  DiagnosticValueWrapper(ReduceTag, T in_value)
    : value_(in_value),
      N_(1),
      min_(in_value),
      max_(in_value),
      sum_(in_value),
      avg_(in_value),
      M2_(0.),
      M3_(0.),
      M4_(0.)
  { }

  /**
   * \internal \brief Get reference to the underlying value
   *
   * \return the value
   */
  T& getRef() { return value_; }

  /**
   * \internal \brief Get const reference to the underlying value
   *
   * \return const ref to value
   */
  T const& getConstRef() const { return value_; }

  /**
   * \internal \brief Operator for reducing over the value
   *
   * \param[in] d1 first operand
   * \param[in] d2 second operand
   *
   * \return value after reducer applied
   */
  friend DiagnosticValueWrapper<T> operator+(
    DiagnosticValueWrapper<T> d1, DiagnosticValueWrapper<T> const& d2
  ) {
    int32_t N            = d1.N_ + d2.N_;
    double delta         = d2.avg_ - d1.avg_;
    double delta_sur_N   = delta / static_cast<double>(N);
    double delta2_sur_N2 = delta_sur_N * delta_sur_N;
    int32_t n2           = d1.N_ * d1.N_;
    int32_t n_c2         = d2.N_ * d2.N_;
    int32_t prod_n       = d1.N_ * d2.N_;
    int32_t n_c          = d2.N_;
    int32_t n            = d1.N_;
    double M2            = d1.M2_;
    double M2_c          = d2.M2_;
    double M3            = d1.M3_;
    double M3_c          = d2.M3_;

    d1.M4_ += d2.M4_
        + prod_n * ( n2 - prod_n + n_c2 ) * delta * delta_sur_N * delta2_sur_N2
        + 6. * ( n2 * M2_c + n_c2 * M2 ) * delta2_sur_N2
        + 4. * ( n * M3_c - n_c * M3 ) * delta_sur_N;

    d1.M3_ += d2.M3_
        + prod_n * ( n - n_c ) * delta * delta2_sur_N2
        + 3. * ( n * M2_c - n_c * M2 ) * delta_sur_N;

    d1.M2_  += d2.M2_ + prod_n * delta * delta_sur_N;
    d1.avg_ += n_c * delta_sur_N;
    d1.N_    = N;
    d1.min_  = std::min(d1.min_, d2.min_);
    d1.max_  = std::max(d1.max_, d2.max_);
    d1.sum_ += d2.sum_;

    return d1;
  }

  /**
   * \internal \brief Get min value (use after reduction)
   *
   * \return the min value
   */
  T max() const { return max_; }

  /**
   * \internal \brief Get sum of values (use after reduction)
   *
   * \return the sum of values
   */
  T sum() const { return sum_; }

  /**
   * \internal \brief Get min of value (use after reduction)
   *
   * \return the min value
   */
  T min() const { return min_; }

  /**
   * \internal \brief Get the mean value (use after reduction)
   *
   * \return the mean value
   */
  double avg() const { return avg_; }

  /**
   * \internal \brief Get the variance (use after reduction)
   *
   * \return the variance
   */
  double var() const { return M2_ * (1.0f / N_); }

  /**
   * \internal \brief Get I---imbalance metric (use after reduction)
   *
   * \return I
   */
  double I() const { return (max() / avg()) - 1.0f; }

  /**
   * \internal \brief Get the standard deviation (use after reduction)
   *
   * \return the standard deviation
   */
  double stdv() const { return std::sqrt(var()); }

  /**
   * \internal \brief Increment the cardinality
   */
  void incrementN() { N_++; }

  /**
   * \internal \brief Get raw underlying value
   */
  T getRawValue() { return value_; }

  /**
   * \internal \brief Get the computed value (based on update type)
   *
   * \note Either returns the mean (when a average update type is applied) or
   * the current value
   */
  T getComputedValue() {
    if (N_ > 0) {
      return value_ / N_;
    } else {
      return value_;
    }
  }

  /**
   * \internal \brief Update the value
   *
   * \param[in] new_val the new value
   */
  void update(T new_val) { value_ = new_val; }

private:
  T value_;                     /**< The raw value */
  std::size_t N_ = 0;           /**< The cardinality */
  T min_, max_, sum_;           /**< The min/max/sum for reduction */
  double avg_, M2_, M3_, M4_;   /**< The avg and 2/3/4 moments for reduction */
};

/**
 * \struct DiagnosticValue
 *
 * \brief A keyed diagnostic value of some type \c T
 */
template <typename T>
struct DiagnosticValue : DiagnosticBase {
  /**
   * \internal \brief Create a new typed diagnostic value
   *
   * \param[in] in_key the key for the value
   * \param[in] in_desc the full description
   * \param[in] in_update the update type for the underlying value
   * \param[in] in_type the diagnostic type
   * \param[in] in_initial_value the initial value
   */
  explicit DiagnosticValue(
    std::string const& in_key, std::string const& in_desc,
    DiagnosticUpdate in_update, DiagnosticTypeEnum in_type,
    T in_initial_value = {}
  ) : DiagnosticBase(in_key, in_desc, in_update, in_type),
      value_(DiagnosticValueWrapper<T>{in_initial_value})
  { }

  /**
   * \internal \brief Update the value
   *
   * \param[in] val new value, updated based on type of \c DiagnosticUpdate
   */
  void update(T val) {
    switch (update_) {
    case DiagnosticUpdate::Replace:
      value_.update(val);
      break;
    case DiagnosticUpdate::Avg:
      value_.incrementN();
    case DiagnosticUpdate::Sum:
      value_.update(value_.getRawValue() + val);
      break;
    }
  }

  /**
   * \internal \brief Get the underlying value
   *
   * \return the value
   */
  T get() const { return value_.getComputedValue(); }

  void reduceOver(Diagnostic* diagnostic, DiagnosticString* out) override;

private:
  DiagnosticValueWrapper<T> value_; /**< The wrapper for the value */
};

}}}} /* end namespace vt::runtime::component::detail */

#endif /*INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_VALUE_H*/
