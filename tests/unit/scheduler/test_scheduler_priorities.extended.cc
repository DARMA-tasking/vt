/*
//@HEADER
// *****************************************************************************
//
//                      test_scheduler_priorities.extended.cc
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

namespace vt { namespace tests { namespace unit {

struct TestSchedPriorities : TestParallelHarness { };

TEST_F(TestSchedPriorities, test_scheduler_priorities_1) {
# if vt_check_enabled(priorities)
  int order = 0, total = 0;

  auto ordering_fn = [&](int expected) {
    vt_debug_print(
      gen, node,
      "term: order={}, expected={}, total={}\n",
      order, expected, total
    );
    EXPECT_EQ(order++, expected);
    total--;
  };

  auto testSched = std::make_unique<vt::sched::Scheduler>();

  // Make a term message
  auto msg = vt::makeMessage<vt::term::TermMsg>(no_epoch);
  vt::theMsg()->markAsTermMessage(msg.get());
  testSched->enqueue(msg, std::bind(ordering_fn, 1));

  // Make a regular message
  auto reg_msg = vt::makeMessage<TestStaticBytesShortMsg<16>>();
  testSched->enqueue(reg_msg, std::bind(ordering_fn, 0));

  total = testSched->workQueueSize();

  do {
    testSched->scheduler();
  } while (total > 0);
# endif
}

TEST_F(TestSchedPriorities, test_scheduler_priorities_2) {
# if vt_check_enabled(priorities)
  int order = 0, total = 0;

  auto ordering_fn = [&](int expected) {
    vt_debug_print(
      gen, node,
      "term: order={}, expected={}, total={}\n",
      order, expected, total
    );
    EXPECT_EQ(order++, expected);
    total--;
  };

  auto testSched = std::make_unique<vt::sched::Scheduler>();

  {
    auto msg = vt::makeMessage<vt::term::TermMsg>(no_epoch);
    vt::messaging::msgSetPriority(msg, sched::Priority::Medium);
    testSched->enqueue(msg, std::bind(ordering_fn, 1));
  }

  {
    auto msg = vt::makeMessage<TestStaticBytesShortMsg<16>>();
    vt::messaging::msgSetPriority(msg, sched::Priority::Low);
    testSched->enqueue(msg, std::bind(ordering_fn, 2));
  }

  {
    auto msg = vt::makeMessage<TestStaticBytesShortMsg<16>>();
    vt::messaging::msgSetPriority(msg, sched::Priority::High);
    testSched->enqueue(msg, std::bind(ordering_fn, 0));
  }

  total = testSched->workQueueSize();

  do {
    testSched->scheduler();
  } while (total > 0);
# endif
}

TEST_F(TestSchedPriorities, test_scheduler_priorities_3) {
# if vt_check_enabled(priorities)
  int order = 0, total = 0;

  auto ordering_fn = [&](int expected) {
    vt_debug_print(
      gen, node,
      "term: order={}, expected={}, total={}\n",
      order, expected, total
    );
    EXPECT_EQ(order++, expected);
    total--;
  };

  auto testSched = std::make_unique<vt::sched::Scheduler>();

  {
    // 2nd-level low priority (should execute first, depth-first)
    auto msg = vt::makeMessage<vt::term::TermMsg>(no_epoch);
    vt::messaging::msgSetPriority(msg, sched::Priority::Low, true);
    testSched->enqueue(msg, std::bind(ordering_fn, 0));
  }

  {
    // 1st-level low priority
    auto msg = vt::makeMessage<TestStaticBytesShortMsg<16>>();
    vt::messaging::msgSetPriority(msg, sched::Priority::Low);
    testSched->enqueue(msg, std::bind(ordering_fn, 2));
  }

  {
    // 1st-level high priority
    auto msg = vt::makeMessage<TestStaticBytesShortMsg<16>>();
    vt::messaging::msgSetPriority(msg, sched::Priority::High);
    testSched->enqueue(msg, std::bind(ordering_fn, 1));
  }

  total = testSched->workQueueSize();

  do {
    testSched->scheduler();
  } while (total > 0);
# endif
}

struct ObjGroup;

struct TestMsg : vt::Message {
  TestMsg(
    vt::objgroup::proxy::Proxy<ObjGroup> in_proxy,
    PriorityType in_priority,
    PriorityLevelType in_priority_level
  ) : priority(in_priority), priority_level(in_priority_level), proxy(in_proxy)
  { }

  PriorityType priority = no_priority;
  PriorityLevelType priority_level = no_priority_level;
  vt::objgroup::proxy::Proxy<ObjGroup> proxy;
};

struct ObjGroup {

  ObjGroup() : testSched(std::make_unique<vt::sched::Scheduler>()) { }

  void enqueue(TestMsg* msg) {

    auto const priority = envelopeGetPriority(msg->env);
    auto const level    = envelopeGetPriorityLevel(msg->env);

    vt_debug_print(
      gen, node,
      "enqueue: priority={:x} vs {:x}, level={:x} vs {:x}, num={}\n",
      msg->priority, priority, msg->priority_level, level,
      vt::sched::priority_num_levels
    );

    EXPECT_EQ(level,    msg->priority_level);
    EXPECT_EQ(priority, msg->priority);

    if (msg->priority_level < vt::sched::priority_num_levels - 1) {
      auto proxy = msg->proxy;
      PriorityType new_priority = msg->priority;
      PriorityLevelType new_level = msg->priority_level + 1;
      sched::PriorityManip::setPriority(
        new_priority, new_level, vt::sched::Priority::MediumLow
      );
      auto new_msg = vt::makeMessage<TestMsg>(proxy, new_priority, new_level);
      vt::messaging::msgSetPriorityFrom(msg, new_msg, vt::sched::Priority::MediumLow, true);

      auto np = vt::envelopeGetPriority(new_msg->env);
      auto npl = vt::envelopeGetPriorityLevel(new_msg->env);

      vt_debug_print(
        gen, node,
        "NEW enqueue: priority={:x}, level={:x}\n", np, npl
      );

      proxy[0].sendMsg<TestMsg,&ObjGroup::enqueue>(new_msg);
    }
  }

private:
  std::unique_ptr<vt::sched::Scheduler> testSched = nullptr;
};

TEST_F(TestSchedPriorities, test_scheduler_priorities_4) {
# if vt_check_enabled(priorities)

  auto proxy = vt::theObjGroup()->makeCollective<ObjGroup>();

  PriorityType p = vt::min_priority;
  PriorityLevelType l = 0;
  proxy[0].send<TestMsg,&ObjGroup::enqueue>(proxy, p, l);

# endif
}

}}} /* end namespace vt::tests::unit */
