/*
//@HEADER
// *****************************************************************************
//
//                     test_scheduler_progress.extended.cc
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

#include "vt/transport.h"
#include "test_parallel_harness.h"

#include <memory>
#include <chrono>
#include <thread>

namespace vt { namespace tests { namespace unit {

struct TestSchedProgress : TestParallelHarness { };

TEST_F(TestSchedProgress, test_scheduler_progress_1) {
  using namespace std::this_thread;
  using namespace std::chrono;
  using namespace std::chrono_literals;

  // Run the progress function every second at least
  vt::theArgConfig()->vt_sched_progress_han = 0;
  vt::theArgConfig()->vt_sched_progress_sec = 1.0;

  bool done = false;

  auto cur_time = vt::timing::Timing::getCurrentTime();
  auto testSched = std::make_unique<vt::sched::Scheduler>();
  testSched->runProgress();

  // Events are polled, create a new event that will be polled in the progress
  // function
  auto event = theEvent()->createParentEvent(theContext()->getNode());
  theEvent()->getEventHolder(event).attachAction([&]{ done = true; });

  // Fill the queue with a second amount of work, in smaller increments to see
  // if progress triggers too early
  for (int i = 0; i < vt::theArgConfig()->vt_sched_progress_sec / 0.05; i++) {
    testSched->enqueue([]{ sleep_for(50ms); });
  }

  do testSched->scheduler(); while (not done);

  double const fudge = 0.8;

  // This ought to take close to a second
  EXPECT_GT(
    vt::timing::Timing::getCurrentTime() - cur_time,
    vt::theArgConfig()->vt_sched_progress_sec * fudge
  );

  // Switch back to default scheduler settings (to not slow down other tests)
  vt::theArgConfig()->vt_sched_progress_han = 0;
  vt::theArgConfig()->vt_sched_progress_sec = 0.0;
}

TEST_F(TestSchedProgress, test_scheduler_progress_2) {
  using namespace std::this_thread;
  using namespace std::chrono;
  using namespace std::chrono_literals;

  // Run scheduler every 10 handlers at least
  vt::theArgConfig()->vt_sched_progress_han = 10;
  vt::theArgConfig()->vt_sched_progress_sec = 0.0;

  bool done = false;

  auto cur_time = vt::timing::Timing::getCurrentTime();
  auto testSched = std::make_unique<vt::sched::Scheduler>();
  testSched->runProgress();

  // Events are polled, create a new event that will be polled in the progress
  // function
  auto event = theEvent()->createParentEvent(theContext()->getNode());
  theEvent()->getEventHolder(event).attachAction([&]{ done = true; });

  for (int i = 0; i < 10; i++) {
    testSched->enqueue([]{ sleep_for(100ms); });
  }

  do testSched->scheduler(); while (not done);

  double const fudge = 0.8;

  // This ought to take close to a second
  EXPECT_GT(
    vt::timing::Timing::getCurrentTime() - cur_time,
    vt::theArgConfig()->vt_sched_progress_sec * fudge
  );

  // Switch back to default scheduler settings (to not slow down other tests)
  vt::theArgConfig()->vt_sched_progress_han = 0;
  vt::theArgConfig()->vt_sched_progress_sec = 0.0;
}


}}} /* end namespace vt::tests::unit */
