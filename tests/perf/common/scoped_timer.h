/*
//@HEADER
// *****************************************************************************
//
//                                scoped_timer.h
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

#if !defined INCLUDED_VT_TEST_PERF_COMMON_SCOPED_TIMER_H
#define INCLUDED_VT_TEST_PERF_COMMON_SCOPED_TIMER_H

#include <string>
#include <chrono>

namespace vt { namespace tests { namespace perf { namespace common {

using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;
using DurationMicro = std::chrono::duration<double, std::micro>;
using DurationMilli = std::chrono::duration<double, std::milli>;
using DurationSec = std::chrono::duration<double>;

struct StopWatch {
  void Start();

  template <typename Duration = DurationMilli>
  Duration Stop() {
    auto const now = std::chrono::steady_clock::now();
    auto const delta = Duration{now - cur_time_};
    cur_time_ = now;

    return delta;
  }

  private:
  TimePoint cur_time_;
};

struct ScopedTimer {
  ScopedTimer(std::string const& name);
  ScopedTimer(std::string && name);
  ~ScopedTimer();

  private:
  std::string name_ = "default";
  StopWatch watch_ = {};
};

}}}} // namespace vt::tests::perf::common

#endif // INCLUDED_VT_TEST_PERF_COMMON_SCOPED_TIMER_H
