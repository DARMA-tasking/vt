/*
//@HEADER
// *****************************************************************************
//
//                         test_scheduler_timings.cc
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

#include <gtest/gtest.h>

#include "vt/scheduler/scheduler.h"
#include "vt/elm/elm_id_bits.h"

#include "test_parallel_harness.h"
#include "test_helpers.h"

#include <memory>
#include <thread>

namespace vt { namespace tests { namespace unit {

struct TestSchedTimings : TestParallelHarness { };
//TODO use TEST_P and have special progress true or false
//make the scheduler run only after it's num_iter

  // // Run scheduler every 10 handlers at least
  // vt::theConfig()->vt_sched_progress_han = num_iter;
  // vt::theConfig()->vt_sched_progress_sec = 0.0;
struct MyMsg : vt::Message {
  int ms = 0;
};

int count = 0;

void myHandler(MyMsg* msg) {
  fmt::print("running myHandler: ms={}\n", msg->ms);
  std::this_thread::sleep_for(std::chrono::milliseconds{msg->ms});
  count++;
}

TEST_F(TestSchedTimings, test_sched_lb) {

  auto sched = std::make_unique<vt::sched::Scheduler>();

  auto const this_node = theContext()->getNode();

  std::vector<std::tuple<int, std::unique_ptr<elm::ElementLBData>>> v;

  int const num_iter = 10;


  for (int i = 0; i < num_iter; i++) {
    int time = i*100;
    v.emplace_back(time, std::make_unique<elm::ElementLBData>());

    auto id = elm::ElmIDBits::createCollection(true, this_node);
    auto handler = auto_registry::makeAutoHandler<MyMsg, myHandler>();
    auto msg = vt::makeMessage<MyMsg>();
    msg->ms = time;
    auto maker = vt::runnable::makeRunnable(msg, false, handler, 0)
      .withLBData(std::get<1>(v[i]).get(), id);
    auto runnable = maker.getRunnableImpl();
    runnable->setupHandler(handler);
    sched->enqueue(false, runnable);
  }

  sched->runSchedulerWhile([&]{ return count < num_iter; });

  for (auto& [time, data] : v) {
    auto load = 1000.0* data->getLoad(0);
    fmt::print("expected time={}, observed time={}\n", time, load);
    double margin = 10+ time*0.10;
    EXPECT_NEAR(time, load, margin );
  }

}

TEST_F(TestSchedTimings, test_sched_msg) {

  SET_MIN_NUM_NODES_CONSTRAINT(2);
  auto sched = std::make_unique<vt::sched::Scheduler>();


  NodeType node = theContext()->getNode();
  NodeType target_node = (node + 1) % theContext()->getNumNodes();

  int const num_iter = 10;
  int const ms_delay = 50;
  count = 0;
  auto start_time = vt::timing::getCurrentTime();

  for (int i = 0; i < num_iter; i++) {

    auto next_msg = vt::makeMessage<MyMsg>();
    next_msg->ms = ms_delay;

    auto handler = auto_registry::makeAutoHandler<MyMsg, myHandler>();

    //theMsg()->sendMsg<MyMsg, myHandler>(target_node, next_msg);
    auto maker = vt::runnable::makeRunnable(next_msg, false, handler, 0);

    auto runnable = maker.getRunnableImpl();
    runnable->setupHandler(handler);
    sched->enqueue(false, runnable);
  }

  sched->runSchedulerWhile([&]{ return count < num_iter; });
  double const fudge = 0.8;
  auto observed_time = 1000.0 *(vt::timing::getCurrentTime() - start_time);

  // This ought to take close to a second (with ms_delay = 100)
  EXPECT_GT(
    observed_time,
    vt::theConfig()->vt_sched_progress_sec * fudge
  );

  auto sum_time = num_iter *ms_delay;
  fmt::print("expected time={}, observed time={}\n", sum_time, observed_time);
  double margin = 10+ sum_time*0.05;
  EXPECT_NEAR(sum_time, observed_time, margin );

}


}}} /* vt::tests::unit */
