/*
//@HEADER
// *****************************************************************************
//
//                           test_perf_data.cc
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

#include "test_parallel_harness.h"
#include <vt/metrics/perf_data.h>
#include <vt/transport.h>

namespace vt { namespace tests { namespace unit {

#if vt_check_enabled(perf)

struct TestPerfData : TestParallelHarness {};

double pi(uint64_t n) {
  double sum = 0.0;
  int sign = 1;
  for (uint64_t i = 0; i < n; ++i) {
    sum += sign/(2.0*i+1.0);
    sign *= -1;
  }
  return 4.0*sum;
}

TEST_F(TestPerfData, DefaultEventName) {
  unsetenv("VT_EVENTS");
  
  vt::metrics::PerfData perf_data;

  auto event_map = perf_data.getEventMap();
  
  ASSERT_NE(event_map.find("instructions"), event_map.end())
      << "Default event 'instructions' should be in the event map.";
}

TEST_F(TestPerfData, ValidCustomEventNames) {
  setenv("VT_EVENTS", "instructions,cache_references", 1);
  
  vt::metrics::PerfData perf_data;

  auto event_map = perf_data.getEventMap();

  EXPECT_NE(event_map.find("instructions"), event_map.end())
      << "Event 'instructions' should be in the event map.";
  EXPECT_NE(event_map.find("cache_references"), event_map.end())
      << "Event 'cache_references' should be in the event map.";
}

TEST_F(TestPerfData, InvalidEventNameTriggersAbort) {
  setenv("VT_EVENTS", "invalid-event", 1);

  EXPECT_THROW(
      vt::metrics::PerfData perf_data,
      std::runtime_error
  ) << "Invalid event names should trigger vtAbort and throw an exception.";
}

TEST_F(TestPerfData, StartAndStopTaskMeasurement) {
  setenv("VT_EVENTS", "instructions", 1);

  vt::metrics::PerfData perf_data;
  
  EXPECT_NO_THROW(perf_data.startTaskMeasurement())
      << "startTaskMeasurement() should not throw an exception.";
  EXPECT_NO_THROW(perf_data.stopTaskMeasurement())
      << "stopTaskMeasurement() should not throw an exception.";
}

TEST_F(TestPerfData, RepeatedStartStop) {
  setenv("VT_EVENTS", "instructions", 1);

  vt::metrics::PerfData perf_data;

  for (int i = 0; i < 100; ++i) {
    EXPECT_NO_THROW(perf_data.startTaskMeasurement());
    EXPECT_NO_THROW(perf_data.stopTaskMeasurement());
  }
}

TEST_F(TestPerfData, GetTaskMeasurements) {
  setenv("VT_EVENTS", "instructions", 1);

  vt::metrics::PerfData perf_data;
  perf_data.startTaskMeasurement();

  double p;
  p = pi(1000);
  fmt::print("pi: {}\n", p);  // print to avoid being optimized out

  perf_data.stopTaskMeasurement();

  auto measurements = perf_data.getTaskMeasurements();

  ASSERT_NE(measurements.find("instructions"), measurements.end())
      << "'instructions' should be in the measurement results.";
  EXPECT_GT(measurements["instructions"], 0)
      << "'instructions' counter should record positive values.";
}

TEST_F(TestPerfData, StartupFunction) {
  setenv("VT_EVENTS", "instructions", 1);

  vt::metrics::PerfData perf_data;

  EXPECT_NO_THROW(perf_data.startup())
      << "startup() should not throw an exception.";
}

TEST_F(TestPerfData, ConstructorDestructorValidation) {
  setenv("VT_EVENTS", "instructions", 1);

  for (int i = 0; i < 100; ++i) {
    EXPECT_NO_THROW({
      vt::metrics::PerfData perf_data;
    }) << "Repeated construction and destruction should not throw errors.";
  }
}


#endif

}}} // end namespace vt::tests::unit