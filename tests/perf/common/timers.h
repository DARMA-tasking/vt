/*
//@HEADER
// *****************************************************************************
//
//                                  timers.h
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

#if !defined INCLUDED_VT_TEST_PERF_COMMON_TIMERS_H
#define INCLUDED_VT_TEST_PERF_COMMON_TIMERS_H

#include <chrono>

namespace vt { namespace tests { namespace perf { namespace common {

using TimePoint = std::chrono::time_point<std::chrono::steady_clock>;

using TimeDuration = double;
using DurationMicro = std::chrono::duration<TimeDuration, std::micro>;
using DurationMilli = std::chrono::duration<TimeDuration, std::milli>;
using DurationSec = std::chrono::duration<TimeDuration>;

template <typename Duration>
struct DurationConverter {
  static DurationMicro ToMicro(Duration const& in_duration) {
    return DurationSec{in_duration};
  }

  static DurationMilli ToMilli(Duration const& in_duration) {
    return DurationMilli{in_duration};
  }

  static DurationSec ToSec(Duration const& in_duration) {
    return DurationSec{in_duration};
  }
};

/*
* Simple stopwatch utility struct for measuring time.
*
* Example use:
*
* StopWatch t;
*
* t.Start();
* SomeFunction();
* auto const delta = t.Stop<DurationMicro>();
*
* fmt::print("SomeFunction took {}?s", delta);
*/
struct StopWatch {
  void Start();

  template <typename Duration = DurationMilli>
  TimeDuration Stop() {
    auto const now = std::chrono::steady_clock::now();
    auto const delta = Duration{now - cur_time_};
    cur_time_ = now;

    return delta.count();
  }

  private:
  TimePoint cur_time_ = std::chrono::steady_clock::now();
};

}}}} // namespace vt::tests::perf::common

#endif // INCLUDED_VT_TEST_PERF_COMMON_TIMERS_H
