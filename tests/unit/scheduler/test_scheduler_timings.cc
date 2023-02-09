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

#include <memory>
#include <thread>

namespace vt { namespace tests { namespace unit {

struct TestSchedTimings : TestParallelHarness { };

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
    int time = i*50;
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
    auto load = data->getLoad(0);
    fmt::print("expected time={}, observed time={}\n", time, load);
  }

}

}}} /* vt::tests::unit */
