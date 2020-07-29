/*
//@HEADER
// *****************************************************************************
//
//                              diagnostic_meter.h
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

#if !defined INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_METER_H
#define INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_METER_H

#include "vt/config.h"

namespace vt { namespace runtime { namespace component {

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
    sum_->update(updated_val);
    avg_->update(updated_val);
    max_->update(updated_val);
    min_->update(updated_val);
  }

protected:
  detail::DiagnosticValue<T>* sum_ = nullptr; /**< Sum of all update values */
  detail::DiagnosticValue<T>* avg_ = nullptr; /**< Avg of all update values */
  detail::DiagnosticValue<T>* max_ = nullptr; /**< Max of all update values */
  detail::DiagnosticValue<T>* min_ = nullptr; /**< Min of all update values */
};

/**
 * \struct Gauge
 *
 * \brief Diagnostic that records some value over time.
 */
template <typename T>
struct Gauge : DiagnosticStatsPack<T> {

  /**
   * \brief Default constructor available for ease of putting this type in a
   * class. But, all valid ways to construction involve the factory methods in
   * the \c Diagnostic base class for component
   */
  Gauge() = default;

private:
  Gauge(
    detail::DiagnosticValue<T>* in_sum,
    detail::DiagnosticValue<T>* in_avg,
    detail::DiagnosticValue<T>* in_max,
    detail::DiagnosticValue<T>* in_min
  ) : DiagnosticStatsPack<T>(in_sum, in_avg, in_max, in_min)
  { }

  friend struct Diagnostic;

public:

  /**
   * \brief Update with a new value
   *
   * \param[in] val the new value
   */
  void update(T val) {
#   if vt_check_enabled(diagnostics)
    this->updateStats(val);
#   endif
  }
};

/**
 * \struct Timer
 *
 * \brief Diagnostic that times some operation over time
 */
template <typename T>
struct Timer : DiagnosticStatsPack<T> {

  /**
   * \brief Default constructor available for ease of putting this type in a
   * class. But, all valid ways to construction involve the factory methods in
   * the \c Diagnostic base class for component
   */
  Timer() = default;

private:
  Timer(
    detail::DiagnosticValue<T>* in_sum,
    detail::DiagnosticValue<T>* in_avg,
    detail::DiagnosticValue<T>* in_max,
    detail::DiagnosticValue<T>* in_min
  ) : DiagnosticStatsPack<T>(in_sum, in_avg, in_max, in_min)
  { }

  friend struct Diagnostic;

public:

  /**
   * \brief Add a new timer range to the timer diagnostic
   *
   * \param[in] begin begin time of event being tracked
   * \param[in] end end time of event being tracked
   */
  void update(T begin, T end) {
#   if vt_check_enabled(diagnostics)
    auto const duration = end - begin;
    this->updateStats(duration);
#   endif
  }
};

/**
 * \struct Counter
 *
 * \brief Diagnostic that counts some quantity over time.
 */
template <typename T>
struct Counter : DiagnosticMeter {

  /**
   * \brief Default constructor available for ease of putting this type in a
   * class. But, all valid ways to construction involve the factory methods in
   * the \c Diagnostic base class for component
   */
  Counter() = default;

private:
  explicit Counter(detail::DiagnosticValue<T>* in_impl)
    : impl_(in_impl)
  { }

  friend struct Diagnostic;

public:
  /**
   * \brief Increment the counter
   *
   * \param[in] val amount to increment
   */
  void increment(T val = 1) {
#   if vt_check_enabled(diagnostics)
    impl_->update(val);
#   endif
  }

  /**
   * \brief Decrement the counter
   *
   * \param[in] val amount to decrement
   */
  void decrement(T val = 1) {
#   if vt_check_enabled(diagnostics)
    impl_->update(-val);
#   endif
  }

private:
  detail::DiagnosticValue<T>* impl_ = nullptr; /**< The actual underlying value */
};

}}} /* end namespace vt::runtime::component */

namespace vt { namespace diagnostic {

template <typename T>
using Counter = runtime::component::Counter<T>;

template <typename T>
using Gauge = runtime::component::Gauge<T>;

template <typename T>
using Timer = runtime::component::Timer<T>;

}} /* end namespace vt;:diagnostic */

#endif /*INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_METER_H*/
