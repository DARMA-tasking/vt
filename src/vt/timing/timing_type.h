/*
//@HEADER
// *****************************************************************************
//
//                                timing_type.h
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

#if !defined INCLUDED_VT_TIMING_TIMING_TYPE_H
#define INCLUDED_VT_TIMING_TIMING_TYPE_H

#include <chrono>
#include <algorithm>
#include <cmath>

namespace vt {

struct TimeTypeWrapper {
  using TimeTypeInternal = double;

  using Seconds = std::chrono::duration<TimeTypeInternal>;
  using Milliseconds = std::chrono::duration<TimeTypeInternal, std::milli>;
  using Microseconds = std::chrono::duration<TimeTypeInternal, std::micro>;

  template <typename T>
  explicit TimeTypeWrapper(
    T time,
    typename std::enable_if<
      std::is_integral<T>::value || std::is_floating_point<T>::value>::type* =
      nullptr)
    : time_(Seconds(static_cast<TimeTypeInternal>(time))) { }

  template <typename Rep = TimeTypeInternal>
  explicit TimeTypeWrapper(
    const std::chrono::duration<Rep, std::micro>& time =
      std::chrono::duration<Rep, std::micro>(0))
    : time_(std::chrono::duration_cast<Seconds>(time)) { }

  template <typename Rep = TimeTypeInternal>
  explicit TimeTypeWrapper(const std::chrono::duration<Rep, std::milli>& time)
    : time_(std::chrono::duration_cast<Seconds>(time)) { }

  template <typename Rep = TimeTypeInternal>
  explicit TimeTypeWrapper(const Seconds& time)
    : time_(std::chrono::duration_cast<Seconds>(time)) { }

  explicit operator TimeTypeInternal() const { return time_.count(); }

  TimeTypeWrapper& operator+=(const TimeTypeWrapper& other) {
    time_ += other.time_;
    return *this;
  }

  TimeTypeWrapper& operator-=(const TimeTypeWrapper& other) {
    time_ -= other.time_;
    return *this;
  }

  friend TimeTypeWrapper
  operator/(const TimeTypeWrapper& lhs, const TimeTypeWrapper& rhs) {
    return TimeTypeWrapper{lhs.time_ / rhs.time_};
  }

  friend TimeTypeWrapper
  operator+(const TimeTypeWrapper& lhs, const TimeTypeWrapper& rhs) {
    return TimeTypeWrapper(lhs.time_ + rhs.time_);
  }

  friend TimeTypeWrapper
  operator-(const TimeTypeWrapper& lhs, const TimeTypeWrapper& rhs) {
    return TimeTypeWrapper(lhs.time_ - rhs.time_);
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

  template <typename Rep = TimeTypeInternal>
  std::chrono::duration<Rep> seconds() const {
    return std::chrono::duration_cast<std::chrono::duration<Rep>>(time_);
  }

  template <typename Rep = TimeTypeInternal>
  std::chrono::duration<Rep, std::milli> milliseconds() const {
    return std::chrono::duration_cast<std::chrono::duration<Rep, std::milli>>(
      time_);
  }

  template <typename Rep = TimeTypeInternal>
  std::chrono::duration<Rep, std::micro> microseconds() const {
    return std::chrono::duration_cast<std::chrono::duration<Rep, std::micro>>(
      time_);
  }

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | time_;
  }

private:
  std::chrono::duration<TimeTypeInternal> time_;
};

using TimeType = TimeTypeWrapper;

} /* end namespace vt */

#endif // INCLUDED_VT_TIMING_TIMING_TYPE_H
