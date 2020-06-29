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
 * \struct Gauge
 *
 * \brief Diagnostic that records some value over time.
 */
template <typename T>
struct Gauge : DiagnosticMeter {

  /**
   * \brief Default constructor available for ease of putting this type in a
   * class. But, all valid ways to construction involve the factory methods in
   * the \c Diagnostic base class for component
   */
  Gauge() = default;

private:
  explicit Gauge(
    detail::DiagnosticValue<T>* in_sum,
    detail::DiagnosticValue<T>* in_avg,
    detail::DiagnosticValue<T>* in_max,
    detail::DiagnosticValue<T>* in_min
  ) : sum_(in_sum),
      avg_(in_avg),
      max_(in_max),
      min_(in_min)
  { }

  friend struct Diagnostic;

public:

  /**
   * \brief Update with a new value
   *
   * \param[in] val the new value
   */
  void update(T val) {
    operator+=(val);
  }

  /**
   * \brief Update with a new value
   *
   * \param[in] val the new value
   */
  Gauge<T>& operator+=(T val) {
    sum_->update(val);
    avg_->update(val);
    min_->update(val);
    max_->update(val);
    return *this;
  }

private:
  detail::DiagnosticValue<T>* sum_ = nullptr; /**< Sum of all gauge values */
  detail::DiagnosticValue<T>* avg_ = nullptr; /**< Avg of all gauge values */
  detail::DiagnosticValue<T>* max_ = nullptr; /**< Max of all gauge values */
  detail::DiagnosticValue<T>* min_ = nullptr; /**< Min of all gauge values */
};

/**
 * \struct Timer
 *
 * \brief Diagnostic that times some operation over time
 */
template <typename T>
struct Timer : DiagnosticMeter {

  /**
   * \brief Default constructor available for ease of putting this type in a
   * class. But, all valid ways to construction involve the factory methods in
   * the \c Diagnostic base class for component
   */
  Timer() = default;

private:
  explicit Timer(detail::DiagnosticValue<T>* in_impl)
    : impl_(in_impl)
  { }

  friend struct Diagnostic;

public:

  void update(T begin, T end) {
    auto const duration = end - begin;
  }

private:
  detail::DiagnosticValue<T>* impl_ = nullptr;
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
  void increment(T val = 1) { impl_->update(val); }

  /**
   * \brief Decrement the counter
   *
   * \param[in] val amount to decrement
   */
  void decrement(T val = 1) { impl_->update(-val); }

  /**
   * \brief Convenience method for incrementing---prefix operator
   */
  Counter<T>& operator++() {
    impl_->update(1);
    return *this;
  }

  /**
   * \brief Convenience method for incrementing---postfix operator
   */
  Counter<T> operator++(int) {
    Counter<T> tmp(*this);
    operator++();
    return tmp;
  }

  /**
   * \brief Convenience method for decrementing---prefix operator
   */
  Counter<T>& operator--() {
    impl_->update(-1);
    return *this;
  }

  /**
   * \brief Convenience method for decrementing---postfix operator
   */
  Counter<T> operator--(int) {
    Counter<T> tmp(*this);
    operator--();
    return tmp;
  }

  /**
   * \brief Convenience method for incrementing by some value
   */
  Counter<T>& operator+=(T val) {
    impl_->update(val);
    return *this;
  }

  /**
   * \brief Convenience method for decrementing by some value
   */
  Counter<T>& operator-=(T val) {
    impl_->update(-val);
    return *this;
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
