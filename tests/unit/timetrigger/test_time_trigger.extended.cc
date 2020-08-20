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

TEST_F(TestTimeTrigger, test_time_trigger) {
  using namespace std::chrono_literals;

  std::chrono::milliseconds trigger_period = 100ms;
  double trigger_period_s =
    std::chrono::duration<double>(trigger_period).count();
  int trigger_id = 42;
  int triggered = 0;

  vt::timetrigger::Trigger trigger{trigger_period, [&](){
    triggered++;
  }, trigger_id};

  EXPECT_EQ(trigger.getID(), trigger_id);
  EXPECT_DOUBLE_EQ(trigger.getLastTriggerTime(), 0.0);
  EXPECT_DOUBLE_EQ(trigger.nextTriggerTime(), 0.0 + trigger_period_s);
  EXPECT_EQ(triggered, 0);

  auto start_time = 4.0;
  trigger.runAction(start_time);

  EXPECT_DOUBLE_EQ(trigger.getLastTriggerTime(), start_time);
  EXPECT_DOUBLE_EQ(trigger.nextTriggerTime(), start_time + trigger_period_s);
  EXPECT_EQ(triggered, 1);
}

TEST_F(TestTimeTrigger, test_time_trigger_manager_1) {
  using namespace std::this_thread;
  using namespace std::chrono;
  using namespace std::chrono_literals;

  std::chrono::milliseconds trigger_period = 100ms;
  double total_time = 2000;

  int triggered = 0;

  auto testTime = std::make_unique<vt::timetrigger::TimeTriggerManager>();
  testTime->progress();

  auto cur_time = vt::timing::Timing::getCurrentTime();

  // register a trigger every 100 milliseconds
  auto id = testTime->addTrigger(trigger_period, [&]{
    triggered++;
  }, true);

  do {
    testTime->progress();
    sleep_for(5ms);
  } while (vt::timing::Timing::getCurrentTime() - cur_time < total_time/1000);

  int tolerance = 15;

  // Allow for some error tolerance in the number of triggers given the period
  EXPECT_LE(triggered, (total_time / trigger_period.count()) + tolerance);
  EXPECT_GE(triggered, (total_time / trigger_period.count()) - tolerance);

  fmt::print("triggered={}\n", triggered);

  // test unregisteration of triggers

  auto prev_triggered = triggered;
  testTime->removeTrigger(id);

  sleep_for(110ms);
  testTime->progress();
  testTime->progress();

  // should not have been triggered again!
  EXPECT_EQ(prev_triggered, triggered);
}

TEST_F(TestTimeTrigger, test_time_trigger_manager_2) {
  using namespace std::chrono_literals;

  std::chrono::milliseconds trigger_period[3] = {100ms, 10ms, 1000ms};
  double total_time = 3000;

  int triggered[3] = { 0, 0, 0 };

  auto testTime = std::make_unique<vt::timetrigger::TimeTriggerManager>();
  testTime->progress();

  auto cur_time = vt::timing::Timing::getCurrentTime();

  for (int i = 0; i < 3; i++) {
    testTime->addTrigger(
      trigger_period[i], [&triggered,i]{
        triggered[i]++;
      },
      true
    );
  }

  do {
    testTime->progress();
  } while (vt::timing::Timing::getCurrentTime() - cur_time < total_time/1000);

  // tolerance of 80% of expected triggers
  double tolerance = 0.8;

  // Allow for some error tolerance in the number of triggers given the period
  for (int i = 0; i < 3; i++) {
    EXPECT_LE(
      triggered[i],
      (total_time / trigger_period[i].count()) + triggered[i] * tolerance
    );
    EXPECT_GE(
      triggered[i],
      (total_time / trigger_period[i].count()) - triggered[i] * tolerance
    );
  }

  for (int i = 0; i < 3; i++) {
    fmt::print("{}: triggered={}\n", trigger_period[i].count(), triggered[i]);
  }
}


}}} /* end namespace vt::tests::unit */
