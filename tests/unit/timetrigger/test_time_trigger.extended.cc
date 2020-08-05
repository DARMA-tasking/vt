/*
//@HEADER
// *****************************************************************************
//
//                        test_time_trigger.extended.cc
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

#include <gtest/gtest.h>

#include <vt/transport.h>
#include <vt/timetrigger/time_trigger_manager.h>

#include "test_parallel_harness.h"

#include <memory>
#include <chrono>
#include <thread>

namespace vt { namespace tests { namespace unit {

using TestTimeTrigger = TestParallelHarness;

TEST_F(TestTimeTrigger, test_time_trigger_1) {
  using namespace std::this_thread;
  using namespace std::chrono;
  using namespace std::chrono_literals;

  std::chrono::milliseconds trigger_period = 100ms;
  double total_time = 2000;

  int triggered = 0;
  std::vector<double> time_offset;

  auto testTime = std::make_unique<vt::timetrigger::TimeTriggerManager>();
  testTime->progress();

  auto cur_time = vt::timing::Timing::getCurrentTime();

  // register a trigger every 100 milliseconds
  auto id = testTime->addTrigger(trigger_period, [&]{
    triggered++;
    time_offset.push_back(vt::timing::Timing::getCurrentTime() - cur_time);
  }, true);

  do {
    testTime->progress();
    sleep_for(5ms);
  } while (vt::timing::Timing::getCurrentTime() - cur_time < total_time/1000);

  int tolerance = 3;

  // Allow for some error tolerance in the number of triggers given the period
  EXPECT_LE(triggered, (total_time / trigger_period.count()) + tolerance);
  EXPECT_GE(triggered, (total_time / trigger_period.count()) - tolerance);

  fmt::print("triggered={}\n", triggered);

  auto iter = time_offset.begin();
  EXPECT_NE(iter, time_offset.end());
  iter++;
  while (iter != time_offset.end()) {
    auto duration_ms = ((*iter) - *(iter-1))*1000;
    EXPECT_LE(duration_ms, trigger_period.count()*1.8);
    EXPECT_GE(duration_ms, trigger_period.count()*0.2);
    fmt::print("duration={}\n", duration_ms);
    iter++;
  }

  // test unregisteration of triggers

  auto prev_triggered = triggered;
  testTime->removeTrigger(id);

  sleep_for(110ms);
  testTime->progress();
  testTime->progress();

  // should not have been triggered again!
  EXPECT_EQ(prev_triggered, triggered);
}

TEST_F(TestTimeTrigger, test_time_trigger_2) {
  using namespace std::chrono_literals;

  std::chrono::milliseconds trigger_period[3] = {100ms, 10ms, 1000ms};
  double total_time = 3000;

  int triggered[3] = { 0, 0, 0 };
  std::vector<std::vector<double>> time_offset;
  time_offset.resize(3);

  auto testTime = std::make_unique<vt::timetrigger::TimeTriggerManager>();
  testTime->progress();

  auto cur_time = vt::timing::Timing::getCurrentTime();

  for (int i = 0; i < 3; i++) {
    testTime->addTrigger(
      trigger_period[i], [&triggered,&time_offset,i,&cur_time]{
        triggered[i]++;
        time_offset[i].push_back(vt::timing::Timing::getCurrentTime() - cur_time);
      },
      true
    );
  }

  do {
    testTime->progress();
  } while (vt::timing::Timing::getCurrentTime() - cur_time < total_time/1000);

  int tolerance = 1;

  // Allow for some error tolerance in the number of triggers given the period
  for (int i = 0; i < 3; i++) {
    EXPECT_LE(triggered[i], (total_time / trigger_period[i].count()) + tolerance);
    EXPECT_GE(triggered[i], (total_time / trigger_period[i].count()) - tolerance);
  }

  for (int i = 0; i < 3; i++) {
    fmt::print("{}: triggered={}\n", trigger_period[i].count(), triggered[i]);
  }

  for (int i = 0; i < 3; i++) {
    auto iter = time_offset[i].begin();
    EXPECT_NE(iter, time_offset[i].end());
    iter++;
    while (iter != time_offset[i].end()) {
      auto duration_ms = ((*iter) - *(iter-1))*1000;
      EXPECT_LE(duration_ms, trigger_period[i].count()*1.8);
      EXPECT_GE(duration_ms, trigger_period[i].count()*0.2);
      fmt::print("duration={}\n", duration_ms);
      iter++;
    }
  }
}


}}} /* end namespace vt::tests::unit */
