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

#include "vt/config.h"
#include "vt/runtime/component/diagnostic_types.h"
#include "vt/runtime/component/diagnostic_erased_value.h"
#include "vt/runtime/component/diagnostic_value_base.h"
#include "vt/utils/adt/histogram_approx.h"

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
  DiagnosticValueWrapper(ReduceTag, T in_value, bool updated, std::size_t in_N)
    : value_(in_value),
      N_(in_N),
      min_(updated ? in_value : std::numeric_limits<T>::max()),
      max_(updated ? in_value : std::numeric_limits<T>::lowest()),
      sum_(updated ? in_value : 0),
      avg_(updated ? in_value : 0),
      M2_(0.),
      M3_(0.),
      M4_(0.),
      hist_(16)
  {
    // add to histogram when starting the reduction
    hist_.add(value_);
  }

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

    d1.hist_.mergeIn(d2.hist_);
    return d1;
  }

  /**
   * \internal \brief Get max value (use after reduction)
   *
   * \return the max value
   */
  T max() const { return N_ == 0 ? 0 : max_; }

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
  T min() const { return N_ == 0 ? 0 : min_; }

  /**
   * \internal \brief Get the arithmetic mean value (use after reduction)
   *
   * \return the mean value
   */
  double avg() const { return N_ == 0 ? 0 : avg_; }

  /**
   * \internal \brief Get the variance (use after reduction)
   *
   * \return the variance
   */
  double var() const { return N_ == 0 ? 0 : (M2_ * (1.0f / N_)); }

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
  double stdv() const { return N_ == 0 ? 0 : std::sqrt(var()); }

  /**
   * \internal \brief Increment the cardinality
   */
  void incrementN() { N_++; }

  /**
   * \internal \brief Get \c N_ the cardinality of the value
   *
   * \return \c N_
   */
  std::size_t getN() const { return N_; }

  /**
   * \internal \brief Get raw underlying value
   */
  T getRawValue() const { return value_; }

  /**
   * \internal \brief Get the computed value (based on update type)
   *
   * \note Either returns the mean (when an average update type is applied) or
   * the current value
   */
  T getComputedValue() const {
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
  void update(T new_val) { value_ = new_val; updated_ = true; }

  /**
   * \internal \brief Serialize this class
   *
   * \param[in] s the serializer
   */
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | value_ | N_ | min_ | max_ | sum_ | avg_ | M2_ | M3_ | M4_ | hist_;
    s | updated_;
  }

  /**
   * \internal \brief Get the histogram after reducing
   *
   * \return the histogram over nodes
   */
  adt::HistogramApprox<double, int64_t> const& getHistogram() const {
    return hist_;
  }

  /**
   * \internal \brief Return if the value was ever updated
   *
   * \return if it was updated
   */
  bool isUpdated() const { return updated_; }

private:
  T value_;                     /**< The raw value */
  std::size_t N_ = 0;           /**< The cardinality */
  T min_ = {}, max_ = {}, sum_ = {}; /**< The min/max/sum for reduction */
  /**
   * The avg and 2/3/4 moments for reduction
   */
  double avg_ = 0.0, M2_ = 0.0, M3_ = 0.0, M4_ = 0.0;
  adt::HistogramApprox<double, int64_t> hist_; /**< Histogram values for reduce */
  bool updated_ = false;        /**< Whether value has changed from initial */
};

/**
 * \struct DiagnosticSnapshotValues
 *
 * \brief A list of diagnostic values that apply over a certain timeframe for a
 * given snapshot
 */
template <typename T>
struct DiagnosticSnapshotValues {

  /**
   * \internal \brief Construct a set of value snapshots
   *
   * \param[in] n_snapshots the number of snapshots
   * \param[in] in_initial_value the initial value for a snapshot
   */
  DiagnosticSnapshotValues(int n_snapshots, T in_initial_value)
    : initial_value_(in_initial_value)
  {
    for (int i = 0; i < n_snapshots; i++) {
      snapshots_.emplace_back(DiagnosticValueWrapper<T>{in_initial_value});
    }
  }

  /**
   * \internal \brief Update the value
   *
   * \param[in] val new value, updated based on type of \c DiagnosticUpdate
   */
  void update(T val, DiagnosticUpdate updater) {
    for (auto& v : snapshots_) {
      switch (updater) {
      case DiagnosticUpdate::Replace:
        v.update(val);
        break;
      case DiagnosticUpdate::Min:
        v.update(std::min(v.getRawValue(), val));
        break;
      case DiagnosticUpdate::Max:
        v.update(std::max(v.getRawValue(), val));
        break;
      case DiagnosticUpdate::Avg:
        v.incrementN();
        // no break to include update for Avg after increment
      case DiagnosticUpdate::Sum:
        v.update(v.getRawValue() + val);
        break;
      default:
        vtAbort("Unknown DiagnosticUpdate---should be unreachable");
        break;
      }
    }
  }

  /**
   * \internal \brief Get a value for a given snapshot
   *
   * \param[in] snapshot the snapshot index
   *
   * \return the value wrapper
   */
  DiagnosticValueWrapper<T>& operator[](int snapshot) {
    return snapshots_.at(snapshot);
  }

  /**
   * \internal \brief Get a value as const for a given snapshot
   *
   * \param[in] snapshot the snapshot index
   *
   * \return the const value wrapper
   */
  DiagnosticValueWrapper<T> const& operator[](int snapshot) const {
    return snapshots_.at(snapshot);
  }

  /**
   * \internal \brief Reset a snapshot.
   *
   * This is typically invoked when a snapshot's timeframe expires and it needs
   * to be reset for computing the value for the next snapshot.
   *
   * \param[in] snapshot the snapshot index
   */
  void reset(int snapshot) {
    snapshots_.at(snapshot) = DiagnosticValueWrapper<T>{initial_value_};
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | initial_value_
      | snapshots_;
  }

protected:
  T initial_value_;                                  /**< Save for later reset */
  std::vector<DiagnosticValueWrapper<T>> snapshots_; /**< Value time snapshots */
};

/**1
 * \struct DiagnosticValue
 *
 * \brief A keyed diagnostic value of some type \c T
 */
template <typename T>
struct DiagnosticValue : DiagnosticBase {
#if !vt_check_enabled(trace_only)
  checkpoint_virtual_serialize_derived_from(DiagnosticBase)
#endif
  /**
   * \internal \brief Create a new typed diagnostic value
   *
   * \param[in] in_key the key for the value
   * \param[in] in_desc the full description
   * \param[in] in_update the update type for the underlying value
   * \param[in] in_unit the unit type for this diagnostic
   * \param[in] in_type the diagnostic type
   * \param[in] in_initial_value the initial value
   */
  DiagnosticValue(
    std::string const& in_key, std::string const& in_desc,
    DiagnosticUpdate in_update, DiagnosticUnit in_unit,
    DiagnosticTypeEnum in_type, T in_initial_value = {}
  ) : DiagnosticBase(in_key, in_desc, in_update, in_unit, in_type),
      values_(DiagnosticSnapshotValues<T>{1, in_initial_value})
  { }

  /**
   * \internal \brief Update the value
   *
   * \param[in] val new value, updated based on type of \c DiagnosticUpdate
   */
  void update(T val) {
    values_.update(val, getUpdateType());
  }

  /**
   * \internal \brief Get the underlying value
   *
   * \return the value
   */
  T get(int snapshot) const { return values_[snapshot].getComputedValue(); }

  /**
   * \internal \brief Implementation of the concrete reduction over \c T
   *
   * \param[in] diagnostic the diagnostic component (for the reducer)
   * \param[out] out the type-erased output
   * \param[in] snapshot the time snapshot to reduce over (0 is entire runtime)
   */
  void reduceOver(
    Diagnostic* diagnostic, DiagnosticErasedValue* out, int snapshot
  ) override;

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | values_;
  }

private:
  DiagnosticSnapshotValues<T> values_; /**< The value snapshots */
};

}}}} /* end namespace vt::runtime::component::detail */

#endif /*INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_VALUE_H*/
