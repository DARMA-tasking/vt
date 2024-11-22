/*
//@HEADER
// *****************************************************************************
//
//                           test_papi_data.cc
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
#include <vt/transport.h>

#if vt_check_enabled(papi)
#include <vt/context/runnable_context/papi_data.h>
#endif

namespace vt { namespace tests { namespace unit {

#if vt_check_enabled(papi)

struct TestPAPIData : TestParallelHarness {};

double pi(uint64_t n) {
  double sum = 0.0;
  int sign = 1;
  for (uint64_t i = 0; i < n; ++i) {
    sum += sign/(2.0*i+1.0);
    sign *= -1;
  }
  return 4.0*sum;
}

TEST_F(TestPAPIData, DefaultEventSet) {
  unsetenv("VT_EVENTS");

  vt::ctx::PAPIData papi_data;

  ASSERT_FALSE(papi_data.native_events.empty())
      << "Default event set should not be empty.";
  EXPECT_EQ(papi_data.native_events[0], "PAPI_TOT_INS")
      << "Default event should be 'PAPI_TOT_INS'.";
}

TEST_F(TestPAPIData, CustomEventSet) {
  setenv("VT_EVENTS", "PAPI_TOT_INS,PAPI_TOT_CYC", 1);

  vt::ctx::PAPIData papi_data;

  ASSERT_EQ(papi_data.native_events.size(), 2)
      << "Custom event set should have two events.";
  EXPECT_EQ(papi_data.native_events[0], "PAPI_TOT_INS")
      << "First event should be 'PAPI_TOT_INS'.";
  EXPECT_EQ(papi_data.native_events[1], "PAPI_TOT_CYC")
      << "Second event should be 'PAPI_TOT_CYC'.";
}

TEST_F(TestPAPIData, InvalidEventTriggersAbort) {
  setenv("VT_EVENTS", "invalid-event", 1);

  EXPECT_THROW(
      vt::ctx::PAPIData(),
      std::runtime_error
  ) << "Invalid event names should trigger vtAbort and throw an exception.";
}

TEST_F(TestPAPIData, StartAndStopMeasurement) {
  setenv("VT_EVENTS", "PAPI_TOT_INS", 1);

  vt::ctx::PAPIData papi_data;

  EXPECT_NO_THROW(papi_data.start())
      << "Calling start() should not throw.";
  EXPECT_NO_THROW(papi_data.stop())
      << "Calling stop() should not throw.";

  EXPECT_GT(papi_data.end_real_cycles, papi_data.start_real_cycles)
      << "Real cycles should increase after measurement.";
  EXPECT_GT(papi_data.end_virt_usec, papi_data.start_virt_usec)
      << "Virtual microseconds should increase after measurement.";
}

TEST_F(TestPAPIData, MeasureMultipleEvents) {
  setenv("VT_EVENTS", "PAPI_TOT_INS,PAPI_TOT_CYC", 1);

  vt::ctx::PAPIData papi_data;

  EXPECT_NO_THROW(papi_data.start())
      << "Calling start() should not throw.";

  double p;
  p = pi(1000);
  fmt::print("pi: {}\n", p);  // print to avoid being optimized out

  EXPECT_NO_THROW(papi_data.stop())
      << "Calling stop() should not throw.";

  ASSERT_EQ(papi_data.values.size(), 2)
      << "Two events should be measured.";
  EXPECT_GT(papi_data.values[0], 0)
      << "First event (PAPI_TOT_INS) should record positive values.";
  EXPECT_GT(papi_data.values[1], 0)
      << "Second event (PAPI_TOT_CYC) should record positive values.";
}

TEST_F(TestPAPIData, MultiplexedEventSet) {
  setenv("VT_EVENTS", "PAPI_TOT_INS,PAPI_L3_TCM,PAPI_BR_INS", 1);

  vt::ctx::PAPIData papi_data;

  ASSERT_EQ(papi_data.native_events.size(), 3)
      << "Custom event set should have three events.";
  EXPECT_NO_THROW(papi_data.start())
      << "Calling start() should not throw.";

  double p;
  p = pi(1000);
  fmt::print("pi: {}\n", p);  // print to avoid being optimized out

  EXPECT_NO_THROW(papi_data.stop())
      << "Calling stop() should not throw.";

  ASSERT_EQ(papi_data.values.size(), 3)
      << "Three events should be measured.";
  for (size_t i = 0; i < papi_data.values.size(); ++i) {
    EXPECT_GT(papi_data.values[i], 0)
        << fmt::format("Event {} should record positive values.", papi_data.native_events[i]);
  }
}

#endif

}}} // end namespace vt::tests::unit