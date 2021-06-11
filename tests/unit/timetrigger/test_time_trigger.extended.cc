/*
//@HEADER
// *****************************************************************************
//
//                        test_time_trigger.extended.cc
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

#include <vt/timetrigger/time_trigger_manager.h>

#include "test_parallel_harness.h"

#include <memory>
#include <chrono>
#include <thread>

namespace vt { namespace tests { namespace unit {

using TestTimeTrigger = TestParallelHarness;

TEST_F(TestTimeTrigger, test_time_trigger) {
  using namespace std::chrono;
  using namespace std::chrono_literals;

  auto trigger_period_ms = 100ms;
  auto trigger_period_s = duration<TimeType>(trigger_period_ms).count();
  int trigger_id = 42;
  int triggered = 0;

  vt::timetrigger::Trigger trigger{trigger_period_ms, [&triggered](){
    triggered++;
  }, trigger_id};

  EXPECT_EQ(trigger.getID(), trigger_id);
  EXPECT_DOUBLE_EQ(trigger.getLastTriggerTime(), 0.0);
  EXPECT_DOUBLE_EQ(trigger.nextTriggerTime(), 0.0 + trigger_period_s);
  EXPECT_EQ(triggered, 0);

  TimeType start_time = 4.0;
  trigger.runAction(start_time);

  EXPECT_DOUBLE_EQ(trigger.getLastTriggerTime(), start_time);
  EXPECT_DOUBLE_EQ(trigger.nextTriggerTime(), start_time + trigger_period_s);
  EXPECT_EQ(trigger.ready(start_time + trigger_period_s), false);
  EXPECT_EQ(trigger.ready(start_time + trigger_period_s + 0.001), true);
  EXPECT_EQ(triggered, 1);
}

TEST_F(TestTimeTrigger, test_time_trigger_manager_add_trigger) {
  using namespace std::chrono_literals;

  TimeType current_time = 5.2;
  auto trigger_period_ms = 100ms;
  int triggered = 0;

  auto trigger_manager =
    std::make_unique<vt::timetrigger::TimeTriggerManager>();

  // trigger every 100 milliseconds
  trigger_manager->addTrigger(current_time, trigger_period_ms, [&]{
    triggered++;
  });
  EXPECT_EQ(triggered, 0);

  // trigger every 100 milliseconds, fire immediately
  trigger_manager->addTrigger(current_time, trigger_period_ms, [&]{
    triggered++;
  }, true);
  EXPECT_EQ(triggered, 1);
}

TEST_F(TestTimeTrigger, test_time_trigger_manager_trigger_ready) {
  using namespace std::chrono;
  using namespace std::chrono_literals;

  TimeType current_time = 5.2;
  auto trigger_period_ms = 100ms;
  auto trigger_period_s = duration<TimeType>(trigger_period_ms).count();
  int triggered = 0;

  auto trigger_manager =
    std::make_unique<vt::timetrigger::TimeTriggerManager>();

  // trigger every 100 milliseconds, fire immediately
  auto id = trigger_manager->addTrigger(current_time, trigger_period_ms, [&]{
    triggered++;
  }, true);
  EXPECT_EQ(triggered, 1);

  trigger_manager->triggerReady(current_time + trigger_period_s);
  EXPECT_EQ(triggered, 1);

  trigger_manager->triggerReady(current_time + trigger_period_s + 0.01);
  EXPECT_EQ(triggered, 2);

  // test unregisteration of triggers
  auto prev_triggered = triggered;
  trigger_manager->removeTrigger(id);
  trigger_manager->triggerReady(current_time + trigger_period_s + 0.01);

  // should not have been triggered again!
  EXPECT_EQ(prev_triggered, triggered);
}

}}} /* end namespace vt::tests::unit */
