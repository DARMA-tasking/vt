/*
//@HEADER
// ************************************************************************
//
//                          benchmark.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_VT_TESTS_BENCHMARK_BENCHMARK_H
#define INCLUDED_VT_TESTS_BENCHMARK_BENCHMARK_H

#include "vt/transport.h"

#include <fmt/format.h>

#include <string>
#include <cassert>

namespace vt { namespace tests { namespace benchmarks {

/*
 * The range of byte sizes used for benchmarking: (2^{min_d}, 2^{max_d}].
 */
enum struct TimeUnits : int8_t {
  Hour, Minute, Second, Milli, Micro, Nano
};

static vt::TimeType minTime(vt::TimeType sec)  { return sec          / 60.0;  }
static vt::TimeType hourTime(vt::TimeType sec) { return minTime(sec) / 60.0;  }
static vt::TimeType secTime(vt::TimeType sec)  { return sec;                  }
static vt::TimeType msTime(vt::TimeType sec)   { return sec         * 1000.0; }
static vt::TimeType usTime(vt::TimeType sec)   { return msTime(sec) * 1000.0; }
static vt::TimeType nsTime(vt::TimeType sec)   { return usTime(sec) * 1000.0; }

static std::string timeUnitStr(TimeUnits unit) {
  switch (unit) {
  case TimeUnits::Hour:   return std::string{"h"};
  case TimeUnits::Minute: return std::string{"min"};
  case TimeUnits::Second: return std::string{"sec"};
  case TimeUnits::Milli:  return std::string{"ms"};
  case TimeUnits::Micro:  return std::string{"us"};
  case TimeUnits::Nano:   return std::string{"ns"};
  default: assert(0);
  }
  return std::string{};
}

static vt::TimeType convertTimeToUnit(TimeUnits unit, vt::TimeType sec) {
  switch (unit) {
  case TimeUnits::Hour:   return hourTime(sec);
  case TimeUnits::Minute: return minTime(sec);
  case TimeUnits::Second: return secTime(sec);
  case TimeUnits::Milli:  return msTime(sec);
  case TimeUnits::Micro:  return usTime(sec);
  case TimeUnits::Nano:   return nsTime(sec);
  default: assert(0);
  }
  return vt::TimeType{};
}

static TimeUnits parseTimeUnit(std::string unit) {
  if (unit == "sec"  or unit == "s") return TimeUnits::Second;
  if (unit == "ms")                  return TimeUnits::Milli;
  if (unit == "us")                  return TimeUnits::Micro;
  if (unit == "ns")                  return TimeUnits::Nano;
  if (unit == "hour" or unit == "h") return TimeUnits::Hour;
  if (unit == "min"  or unit == "m") return TimeUnits::Minute;
  return TimeUnits::Second;
}

static constexpr bool    const e_not     = false;
static constexpr int64_t const max_d     = 30;
static constexpr int64_t const min_d     = 0;
static constexpr int64_t const min_bytes = 1ull << min_d;
static constexpr int64_t const max_bytes = 1ull << max_d;
static constexpr int64_t const min_k     = 1;

constexpr int64_t numBytes(int64_t const d) { return 1ull << d; }

struct Benchmark {
  template <int64_t d1, int64_t d2> using LessD = std::enable_if_t<d1<d2>;
  template <int64_t d1, int64_t d2> using DoneD = std::enable_if_t<d1>=d2>;

  Benchmark(
    std::string const& in_name,
    int64_t const& in_user_k = min_k,
    int64_t const& in_max_d = max_d,
    std::string unit = ""
  ) : benchmark_(in_name),
      time_unit_(unit != "" ? parseTimeUnit(unit) : TimeUnits::Milli),
      this_node_(vt::theContext()->getNode()),
      num_nodes_(vt::theContext()->getNumNodes()),
      node_next_(this_node_ + 1),
      ring_node_(node_next_ < num_nodes_ ? node_next_ : 0),
      cur_k_(std::max(min_k,in_user_k)),
      max_user_d_(in_max_d == 0 ? max_d : in_max_d)
  { }

  virtual ~Benchmark() = default;

  virtual void start() = 0;

  void print(int64_t d, int64_t k, bool last = false) {
    auto const& vt_header  = std::string{"vt (Virtual Transport) Benchmarking"};
    auto const& cur_time   = vt::timing::Timing::getCurrentTime();
    auto const& total_time = cur_time - start_time_;
    auto const& space      = std::string{" "};
#   define MAX_WIDTH "80"
    if (d == 0) {
      auto time_unit_str = timeUnitStr(time_unit_);
      auto time_str      = fmt::format("t ({})",         time_unit_str);
      auto time_k_str    = fmt::format("t/k ({})",       time_unit_str);
      auto time_nk_str   = fmt::format("t/k/(n-1) ({})", time_unit_str);
      auto time_kb_str   = fmt::format("t/k*b (Kb/{})",  time_unit_str);
      auto node_str      = fmt::format("n={}",           num_nodes_);
      fmt::print(
        " {:_^" MAX_WIDTH "} \n"
        "|{: ^" MAX_WIDTH "}|\n"
        "|{: ^" MAX_WIDTH "}|\n"
        "|{: ^" MAX_WIDTH "}|\n"
        "|{: ^" MAX_WIDTH "}|\n"
        "|{: ^" MAX_WIDTH "}|\n"
        "|{:-^" MAX_WIDTH "}|\n",
        "", "", vt_header, benchmark_, node_str, "", ""
      );

      fmt::print(
        "|  {:^1} |{:^8}|{:^15}|{:^16}|{:^16}|{:^16}|\n",
        "d", "k", "b (bytes)", time_k_str, time_nk_str, time_kb_str
      );

      fmt::print(
        "|{:-^" MAX_WIDTH "}|\n", ""
      );
    }
    auto const& b               = numBytes(d);
    auto const& kb              = numBytes(d) / 1000.0;
    auto const& gb              = numBytes(d) / 1000.0 / 1000.0;
    auto const& n               = num_nodes_ - 1 > 0 ? num_nodes_ - 1 : 1;
    auto const& unit_total_time = convertTimeToUnit(time_unit_,total_time);
    auto const& unit_k_time     = convertTimeToUnit(time_unit_,total_time/k);
    auto const& unit_kn_time    = convertTimeToUnit(time_unit_,total_time/k/n);
    auto const& unit_kb_time    = convertTimeToUnit(time_unit_,total_time/k*kb);
    auto const& fmt_total_time  = fmt::format("{:.7g}", unit_total_time, 9);
    auto const& fmt_avg_time    = fmt::format("{:.7g}", unit_k_time,     9);
    auto const& fmt_avgn_time   = fmt::format("{:.7g}", unit_kn_time,    9);
    auto const& fmt_avgb_time   = fmt::format("{:.7g}", unit_kb_time,    9);
    fmt::print(
      "| {:>2} |{:^8}| {:>13} |  {:<12}  |  {:<12}  |  {:<12}  |\n",
      d, k, b, fmt_avg_time, fmt_avgn_time, fmt_avgb_time
    );
    if (last) {
      fmt::print("|{:_^" MAX_WIDTH "}|\n", "");
    }
#   undef MAX_WIDTH
  }

protected:
  std::string benchmark_   = "";
  TimeUnits   time_unit_   = TimeUnits::Milli;
  vt::NodeType this_node_  = vt::uninitialized_destination;
  vt::NodeType num_nodes_  = vt::uninitialized_destination;
  vt::NodeType node_next_  = vt::uninitialized_destination;
  vt::NodeType ring_node_  = vt::uninitialized_destination;
  int64_t      max_user_d_ = max_d;
  int64_t      cur_k_      = min_k;
  vt::TimeType start_time_ = 0.0;
};

template <int64_t d>
struct BlockDataMsg : vt::ShortMessage {
  BlockDataMsg() = default;
  explicit BlockDataMsg(int64_t const in_k) : k_(in_k) { }
  int64_t k() const { return k_; }
private:
  int64_t k_ = min_k;
  std::array<char, numBytes(d)> payload_;
};

}}} /* end namespace vt::tests::benchmarks */

#endif /*INCLUDED_VT_TESTS_BENCHMARK_BENCHMARK_H*/
