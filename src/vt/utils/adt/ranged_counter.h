/*
//@HEADER
// *****************************************************************************
//
//                               ranged_counter.h
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

#if !defined INCLUDED_VT_UTILS_ADT_RANGED_COUNTER_H
#define INCLUDED_VT_UTILS_ADT_RANGED_COUNTER_H

namespace vt { namespace util { namespace adt {

/**
 * \struct RangedCounter
 *
 * \brief A counter that operates over a fixed range and always stays within the
 * range without overflowing (wraps around).
 */
template <typename T>
struct RangedCounter {

  /**
   * \brief Create a new \c RangedCounter with inclusive bounds
   *
   * \param[in] in_lo the low bound
   * \param[in] in_hi the high bound
   */
  RangedCounter(T in_lo, T in_hi)
    : lo_(in_lo),
      hi_(in_hi),
      cur_(lo_)
  { }

  /**
   * \brief Get the current value (always within the range)
   *
   * \return the current value
   */
  inline T get() const { return cur_; }

  /**
   * \brief Get the current value as a conversion
   */
  inline operator T() const { return get(); }

  /**
   * \brief Pre-increment operator
   */
  RangedCounter<T>& operator++() {
    increment();
    return *this;
  }

  /**
   * \brief Post-increment operator
   */
  RangedCounter<T> operator++(int) {
    RangedCounter<T> copy(*this);
    operator++();
    return copy;
  }

  /**
   * \brief Pre-decrement operator
   */
  RangedCounter<T>& operator--() {
    decrement();
    return *this;
  }

  /**
   * \brief Post-decrement operator
   */
  RangedCounter<T> operator--(int) {
    RangedCounter<T> copy(*this);
    operator--();
    return copy;
  }

  /**
   * \brief Increment the value, keeping it within the range; wraps around if
   * \c cur_ == hi_
   */
  inline void increment() {
    if (cur_ < hi_) {
      cur_++;
    } else {
      cur_ = lo_;
    }
  }

  /**
   * \brief Decrement the value, keeping it within the range; wraps around if
   * \c cur_ == lo_
   */
  inline void decrement() {
    if (cur_ > lo_) {
      cur_--;
    } else {
      cur_ = hi_;
    }
  }

private:
  T lo_ = {};             /**< The low value for inclusive range */
  T hi_ = {};             /**< The high value for the inclusive range*/
  T cur_ = {};            /**< The current value, always within the range */
};

}}} /* end namespace vt::util::adt */

namespace vt { namespace adt {

template <typename T>
using RangedCounter = vt::util::adt::RangedCounter<T>;

}} /* end namespace vt::adt */

#endif /*INCLUDED_VT_UTILS_ADT_RANGED_COUNTER_H*/
