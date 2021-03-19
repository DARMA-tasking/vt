/*
//@HEADER
// *****************************************************************************
//
//                       test_priority_queue.extended.cc
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

namespace vt { namespace tests { namespace unit {

struct TestPriorityQueue : TestParallelHarness { };

TEST_F(TestPriorityQueue, test_priority_queue_1) {
  using PriorityUnit = vt::sched::PriorityUnit;

  int seq = 1;
  vt::sched::PriorityQueue<PriorityUnit> queue;
  bool const t = false;

  queue.emplace(PriorityUnit(t, [&]{ EXPECT_EQ(seq, 5); seq++; }, 1));
  queue.emplace(PriorityUnit(t, [&]{ EXPECT_EQ(seq, 4); seq++; }, 2));
  queue.emplace(PriorityUnit(t, [&]{ EXPECT_EQ(seq, 3); seq++; }, 3));
  queue.emplace(PriorityUnit(t, [&]{ EXPECT_EQ(seq, 2); seq++; }, 4));
  queue.emplace(PriorityUnit(t, [&]{ EXPECT_EQ(seq, 1); seq++; }, 5));

  EXPECT_EQ(seq, 1);

  while (not queue.empty()) {
    queue.pop()();
  }

  EXPECT_EQ(seq, 6);
}

TEST_F(TestPriorityQueue, test_priority_queue_2) {
  using PriorityUnit = vt::sched::PriorityUnit;

  int seq = 1;
  vt::sched::PriorityQueue<PriorityUnit> queue;
  bool const t = false;

  queue.emplace(PriorityUnit(t, [&]{ EXPECT_EQ(seq, 4); seq++; }, 2));
  queue.emplace(PriorityUnit(t, [&]{ EXPECT_EQ(seq, 3); seq++; }, 3));
  queue.emplace(PriorityUnit(t, [&]{ EXPECT_EQ(seq, 5); seq++; }, 1));
  queue.emplace(PriorityUnit(t, [&]{ EXPECT_EQ(seq, 1); seq++; }, 5));
  queue.emplace(PriorityUnit(t, [&]{ EXPECT_EQ(seq, 2); seq++; }, 4));

  EXPECT_EQ(seq, 1);

  while (not queue.empty()) {
    queue.pop()();
  }

  EXPECT_EQ(seq, 6);
}

TEST_F(TestPriorityQueue, test_priority_queue_3) {
  using PriorityUnit = vt::sched::PriorityUnit;
  using namespace vt::sched;

  int seq = 1;
  vt::sched::PriorityQueue<PriorityUnit> queue;
  bool const t = false;

#if vt_feature_cmake_priority_bits_level == 1
  queue.emplace(PriorityUnit(t, [&]{ EXPECT_EQ(seq, 2); seq++; }, Priority::DepthFirst));
  queue.emplace(PriorityUnit(t, [&]{ EXPECT_EQ(seq, 1); seq++; }, Priority::BreadthFirst));
#elif vt_feature_cmake_priority_bits_level == 2
  queue.emplace(PriorityUnit(t, [&]{ EXPECT_EQ(seq, 4); seq++; }, Priority::Low));
  queue.emplace(PriorityUnit(t, [&]{ EXPECT_EQ(seq, 3); seq++; }, Priority::Medium));
  queue.emplace(PriorityUnit(t, [&]{ EXPECT_EQ(seq, 2); seq++; }, Priority::High));
  queue.emplace(PriorityUnit(t, [&]{ EXPECT_EQ(seq, 1); seq++; }, Priority::BreadthFirst));
#elif vt_feature_cmake_priority_bits_level == 3
  queue.emplace(PriorityUnit(t, [&]{ EXPECT_EQ(seq, 8); seq++; }, Priority::Lowest));
  queue.emplace(PriorityUnit(t, [&]{ EXPECT_EQ(seq, 7); seq++; }, Priority::Low));
  queue.emplace(PriorityUnit(t, [&]{ EXPECT_EQ(seq, 6); seq++; }, Priority::MediumLow));
  queue.emplace(PriorityUnit(t, [&]{ EXPECT_EQ(seq, 5); seq++; }, Priority::Medium));
  queue.emplace(PriorityUnit(t, [&]{ EXPECT_EQ(seq, 4); seq++; }, Priority::MediumHigh));
  queue.emplace(PriorityUnit(t, [&]{ EXPECT_EQ(seq, 3); seq++; }, Priority::High));
  queue.emplace(PriorityUnit(t, [&]{ EXPECT_EQ(seq, 2); seq++; }, Priority::Highest));
  queue.emplace(PriorityUnit(t, [&]{ EXPECT_EQ(seq, 1); seq++; }, Priority::BreadthFirst));
#else
  // Do nothing
#endif


  EXPECT_EQ(seq, 1);

  while (not queue.empty()) {
    queue.pop()();
  }

#if vt_feature_cmake_priority_bits_level == 1
  EXPECT_EQ(seq, 3);
#elif vt_feature_cmake_priority_bits_level == 2
  EXPECT_EQ(seq, 5);
#elif vt_feature_cmake_priority_bits_level == 3
  EXPECT_EQ(seq, 9);
#else
  EXPECT_EQ(seq, 1);
#endif
}

}}} /* end namespace vt::tests::unit */
