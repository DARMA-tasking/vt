/*
//@HEADER
// *****************************************************************************
//
//                                timing_type.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_TIMING_TIMING_TYPE_H
#define INCLUDED_VT_TIMING_TIMING_TYPE_H

#include <limits>
#include <algorithm>
#include <cmath>
#include <string>

#include <EngFormat-Cpp/eng_format.hpp>

#include "vt/cmake_config.h"
#include INCLUDE_FMT_BASE

namespace vt {

struct TimeTypeWrapper {
  using TimeTypeInternal = double;
  explicit constexpr TimeTypeWrapper(const TimeTypeInternal time = 0.0)
    : time_(time) { }

  explicit operator double() const { return time_; }

  TimeTypeWrapper& operator+=(const TimeTypeWrapper& other) {
    time_ += other.time_;
    return *this;
  }

  TimeTypeWrapper& operator-=(const TimeTypeWrapper& other) {
    time_ -= other.time_;
    return *this;
  }

  TimeTypeWrapper& operator*=(const double scalar) {
    time_ *= scalar;
    return *this;
  }

  TimeTypeWrapper& operator/=(const double scalar) {
    time_ /= scalar;
    return *this;
  }

  friend TimeTypeWrapper
  operator+(const TimeTypeWrapper& lhs, const TimeTypeWrapper& rhs) {
    return TimeTypeWrapper(lhs.time_ + rhs.time_);
  }

  friend TimeTypeWrapper
  operator-(const TimeTypeWrapper& lhs, const TimeTypeWrapper& rhs) {
    return TimeTypeWrapper(lhs.time_ - rhs.time_);
  }

  friend TimeTypeWrapper
  operator*(const TimeTypeWrapper& lhs, const TimeTypeWrapper& rhs) {
    return TimeTypeWrapper(lhs.time_ * rhs.time_);
  }

  friend TimeTypeWrapper
  operator*(const TimeTypeWrapper& time, const double scalar) {
    return TimeTypeWrapper(time.time_ * scalar);
  }

  friend TimeTypeWrapper
  operator*(const double scalar, const TimeTypeWrapper& time) {
    return TimeTypeWrapper(time.time_ * scalar);
  }

  friend TimeTypeWrapper
  operator/(const double scalar, const TimeTypeWrapper& time) {
    return TimeTypeWrapper(scalar / time.time_);
  }

  friend TimeTypeWrapper
  operator/(const TimeTypeWrapper& time, const double scalar) {
    return TimeTypeWrapper(time.time_ / scalar);
  }

  friend TimeTypeWrapper
  operator/(const TimeTypeWrapper& lhs, const TimeTypeWrapper& rhs) {
    return TimeTypeWrapper(lhs.time_ / rhs.time_);
  }

  friend bool
  operator<(const TimeTypeWrapper& lhs, const TimeTypeWrapper& rhs) {
    return lhs.time_ < rhs.time_;
  }

  friend bool
  operator<=(const TimeTypeWrapper& lhs, const TimeTypeWrapper& rhs) {
    return lhs.time_ <= rhs.time_;
  }

  friend bool
  operator>(const TimeTypeWrapper& lhs, const TimeTypeWrapper& rhs) {
    return lhs.time_ > rhs.time_;
  }

  friend bool
  operator>=(const TimeTypeWrapper& lhs, const TimeTypeWrapper& rhs) {
    return lhs.time_ >= rhs.time_;
  }

  friend bool
  operator==(const TimeTypeWrapper& lhs, const TimeTypeWrapper& rhs) {
    return lhs.time_ == rhs.time_;
  }

  friend bool
  operator!=(const TimeTypeWrapper& lhs, const TimeTypeWrapper& rhs) {
    return lhs.time_ != rhs.time_;
  }

  friend TimeTypeWrapper sqrt(const TimeTypeWrapper& time) {
    return TimeTypeWrapper{std::sqrt(time.time_)};
  }

  TimeTypeInternal seconds() const { return time_; }
  TimeTypeInternal milliseconds() const { return time_ * 1000; }
  TimeTypeInternal microseconds() const { return time_ * 1000000; }

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | time_;
  }

private:
  TimeTypeInternal time_;
};

inline auto format_as(TimeTypeWrapper t) {
  return to_engineering_string(t.seconds(), 5, eng_exponential, "s");
}

using TimeType = TimeTypeWrapper;

} /* end namespace vt */

namespace std {
template <>
class numeric_limits<vt::TimeTypeWrapper> {
  using Type = typename vt::TimeTypeWrapper::TimeTypeInternal;

public:
  static constexpr vt::TimeTypeWrapper max() noexcept {
    return vt::TimeTypeWrapper(std::numeric_limits<Type>::max());
  }

  static constexpr vt::TimeTypeWrapper lowest() noexcept {
    return vt::TimeTypeWrapper(std::numeric_limits<Type>::lowest());
  }

  inline vt::TimeTypeWrapper
  min(const vt::TimeTypeWrapper& lhs, const vt::TimeTypeWrapper& rhs) {
    return vt::TimeTypeWrapper(std::min(lhs.seconds(), rhs.seconds()));
  }

  inline vt::TimeTypeWrapper
  max(const vt::TimeTypeWrapper& lhs, const vt::TimeTypeWrapper& rhs) {
    return vt::TimeTypeWrapper(std::max(lhs.seconds(), rhs.seconds()));
  }
};
} // namespace std

#endif /*INCLUDED_VT_TIMING_TIMING_TYPE_H*/
