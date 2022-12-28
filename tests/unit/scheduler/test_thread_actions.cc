/*
//@HEADER
// *****************************************************************************
//
//                            test_thread_actions.cc
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

#include <mpi.h>
#include "test_parallel_harness.h"

#include <vt/messaging/async_op_mpi.h>
#include <vt/messaging/active.h>
#include <vt/objgroup/headers.h>
#include <vt/transport.h>

#include <gtest/gtest.h>

#if vt_check_enabled(fcontext)

namespace vt { namespace tests { namespace unit { namespace scheduler {

using TestThreadAction = TestParallelHarness;

using TA = vt::sched::ThreadAction;

namespace test1 {

ThreadIDType t1 = 0;
ThreadIDType t2 = 0;

void task1() {
  EXPECT_EQ(TA::getActiveThreadID(), t1);
  vt_print(gen, "task1 start\n");
  TA::suspend();
  EXPECT_EQ(TA::getActiveThreadID(), t1);
  TA::suspend();
  EXPECT_EQ(TA::getActiveThreadID(), t1);
  vt_print(gen, "task1 end\n");
}

void task2() {
  EXPECT_EQ(TA::getActiveThreadID(), t2);
  vt_print(gen, "task2 start\n");
  TA::suspend();
  EXPECT_EQ(TA::getActiveThreadID(), t2);
  TA::suspend();
  EXPECT_EQ(TA::getActiveThreadID(), t2);
  vt_print(gen, "task2 end\n");
}

TEST_F(TestThreadAction, test_thread_action_1) {
  EXPECT_EQ(TA::getActiveThreadID(), no_thread_id);

  auto tm = theSched()->getThreadManager();

  t1 = tm->allocateThread(task1);
  t2 = tm->allocateThread(task2);
  vt_print(gen, "t1={}, t2={}\n", t1, t2);

  EXPECT_EQ(TA::getActiveThreadID(), no_thread_id);

  auto ta1 = tm->getThread(t1);
  auto ta2 = tm->getThread(t2);

  ta1->run();
  EXPECT_EQ(TA::getActiveThreadID(), no_thread_id);
  ta2->run();
  EXPECT_EQ(TA::getActiveThreadID(), no_thread_id);

  while (not ta1->isDone() or not ta2->isDone()) {
    if (not ta1->isDone()) {
      ta1->resume();
      EXPECT_EQ(TA::getActiveThreadID(), no_thread_id);
    } else {
      ta2->resume();
      EXPECT_EQ(TA::getActiveThreadID(), no_thread_id);
    }
  }
}

} /* end namespace test1 */

namespace test2 {

ThreadIDType t1 = 0;
ThreadIDType t2 = 0;

void task2();

void task1() {
  EXPECT_EQ(TA::getActiveThreadID(), t1);
  vt_print(gen, "task1 start\n");

  auto tm = theSched()->getThreadManager();
  t2 = tm->allocateThread(task2);
  auto ta2 = tm->getThread(t2);
  ta2->run();

  TA::suspend();
  EXPECT_EQ(TA::getActiveThreadID(), t1);
  TA::suspend();
  EXPECT_EQ(TA::getActiveThreadID(), t1);
  vt_print(gen, "task1 end\n");
}

void task2() {
  EXPECT_EQ(TA::getActiveThreadID(), t2);
  vt_print(gen, "task2 start\n");
  TA::suspend();
  EXPECT_EQ(TA::getActiveThreadID(), t2);
  TA::suspend();
  EXPECT_EQ(TA::getActiveThreadID(), t2);
  vt_print(gen, "task2 end\n");
}

TEST_F(TestThreadAction, test_thread_action_nested_2) {

  auto tm = theSched()->getThreadManager();
  t1 = tm->allocateThread(task1);
  vt_print(gen, "t1={}\n", t1);

  auto ta1 = tm->getThread(t1);
  ta1->run();
  EXPECT_EQ(TA::getActiveThreadID(), no_thread_id);

  auto ta2 = tm->getThread(t2);
  while (not ta1->isDone() or not ta2->isDone()) {
    if (not ta1->isDone()) {
      ta1->resume();
      EXPECT_EQ(TA::getActiveThreadID(), no_thread_id);
    } else {
      ta2->resume();
      EXPECT_EQ(TA::getActiveThreadID(), no_thread_id);
    }
  }
}

} /* end namespace test2 */

}}}} /* end namespace vt::tests::unit::scheduler */

#endif /*vt_check_enabled(fcontext)*/
