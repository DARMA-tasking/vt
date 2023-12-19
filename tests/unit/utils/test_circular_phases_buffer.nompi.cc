/*
//@HEADER
// *****************************************************************************
//
//                     test_circular_phases_buffer.nompi.cc
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

#include <vt/utils/container/circular_phases_buffer.h>
#include "test_harness.h"

namespace vt { namespace tests { namespace unit {

using TestCircularPhasesBuffer = TestHarness;

struct Dummy {
  int x;
};

void validatePresentPhases(
  util::container::CircularPhasesBuffer<Dummy>& buffer,
  std::vector<PhaseType> expected) {
  for (auto&& phase : expected) {
    EXPECT_TRUE(buffer.contains(phase)) << "phase: " << phase;
    EXPECT_EQ(phase * phase, buffer[phase].x) << "phase: " << phase;
  }
}

void validateMissingPhases(
  util::container::CircularPhasesBuffer<Dummy>& buffer,
  std::vector<PhaseType> expected) {
  for (auto&& phase : expected) {
    EXPECT_FALSE(buffer.contains(phase)) << "phase: " << phase;
  }
}

TEST_F(TestCircularPhasesBuffer, test_circular_phases_buffer_empty) {
  util::container::CircularPhasesBuffer<Dummy> buffer;

  EXPECT_FALSE(buffer.isInitialized());
  EXPECT_EQ(std::size_t{0}, buffer.capacity());
  EXPECT_FALSE(buffer.contains(3));
  EXPECT_FALSE(buffer.contains(10));
  EXPECT_EQ(std::size_t{0}, buffer.size());

  buffer.resize(2);

  EXPECT_TRUE(buffer.isInitialized());
  EXPECT_EQ(std::size_t{2}, buffer.capacity());
  EXPECT_FALSE(buffer.contains(3));
  EXPECT_FALSE(buffer.contains(10));
  EXPECT_EQ(std::size_t{0}, buffer.size());

  buffer.clear();

  EXPECT_TRUE(buffer.isInitialized());
  EXPECT_EQ(std::size_t{2}, buffer.capacity());
  EXPECT_FALSE(buffer.contains(3));
  EXPECT_FALSE(buffer.contains(10));
  EXPECT_EQ(std::size_t{0}, buffer.size());

  buffer.resize(0);

  EXPECT_FALSE(buffer.isInitialized());
  EXPECT_EQ(std::size_t{0}, buffer.capacity());
  EXPECT_FALSE(buffer.contains(3));
  EXPECT_FALSE(buffer.contains(10));
  EXPECT_EQ(std::size_t{0}, buffer.size());
}

TEST_F(TestCircularPhasesBuffer, test_circular_phases_buffer_store) {
  util::container::CircularPhasesBuffer<Dummy> buffer{10};

  buffer.store(2, {2 * 2});
  validatePresentPhases(buffer, {2});

  // store series of elements
  for (int i = 0; i < 15; i++) {
    buffer.store(i, {i * i});
  }
  std::vector<PhaseType> finalOutput = {5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
  validatePresentPhases(buffer, finalOutput);

  std::vector<PhaseType> visited;
  for (auto&& ele : buffer) {
    visited.push_back(ele.first);
    EXPECT_EQ(ele.first * ele.first, ele.second.x) << "phase: " << ele.first;
  }
  EXPECT_TRUE(std::equal(visited.begin(), visited.end(), finalOutput.begin()));
}

TEST_F(
  TestCircularPhasesBuffer, test_circular_phases_buffer_resize_continuous) {
  util::container::CircularPhasesBuffer<Dummy> buffer{1};

  buffer[0] = {0};
  validatePresentPhases(buffer, {0});

  buffer.resize(10);
  validatePresentPhases(buffer, {0});

  for (int i = 0; i <= 15; i++) {
    buffer[i] = {i * i};
  }
  validatePresentPhases(buffer, {6, 7, 8, 9, 10, 11, 12, 13, 14, 15});

  buffer.resize(5);
  validatePresentPhases(buffer, {11, 12, 13, 14, 15});
  validateMissingPhases(buffer, {6, 7, 8, 9, 10});

  for (int i = 15; i <= 32; i++) {
    buffer[i] = {i * i};
  }
  validatePresentPhases(buffer, {28, 29, 30, 31, 32});
  validateMissingPhases(buffer, {11, 12, 13, 14, 15});

  buffer.resize(9);

  for (int i = 33; i <= 35; i++) {
    buffer[i] = {i * i};
  }
  validatePresentPhases(buffer, {28, 29, 30, 31, 32, 33, 34, 35});

  buffer.resize(1);
  validatePresentPhases(buffer, {35});
  validateMissingPhases(buffer, {28, 29, 30, 31, 32, 33, 34, 35});
}

TEST_F(TestCircularPhasesBuffer, test_circular_phases_buffer_forward_iter) {
  util::container::CircularPhasesBuffer<Dummy> buffer;

  for (auto&& ele : buffer) {
    // This will never be called.
    EXPECT_EQ(true, false) << "Unexpected phase: " << ele.first;
  }

  buffer.resize(4);

  for (auto&& ele : buffer) {
    // This will never be called.
    EXPECT_EQ(true, false) << "Unexpected phase: " << ele.first;
  }

  buffer[0] = {0};
  buffer[1] = {1};
  buffer[2] = {4};

  {
    std::vector<PhaseType> expected = {0, 1, 2};
    std::vector<PhaseType> visited;
    for (auto&& ele : buffer) {
      visited.push_back(ele.first);
      EXPECT_EQ(ele.first * ele.first, ele.second.x);
    }
    EXPECT_TRUE(std::equal(visited.begin(), visited.end(), expected.begin()));
  }

  buffer[3] = {9};

  {
    std::vector<PhaseType> expected = {0, 1, 2, 3};
    std::vector<PhaseType> visited;
    for (auto&& ele : buffer) {
      visited.push_back(ele.first);
      EXPECT_EQ(ele.first * ele.first, ele.second.x);
    }
    EXPECT_TRUE(std::equal(visited.begin(), visited.end(), expected.begin()));
  }

  buffer[4] = {16};
  buffer[5] = {25};
  buffer[6] = {36};

  {
    std::vector<PhaseType> expected = {3, 4, 5, 6};
    std::vector<PhaseType> visited;
    for (auto&& ele : buffer) {
      visited.push_back(ele.first);
      EXPECT_EQ(ele.first * ele.first, ele.second.x);
    }
    EXPECT_TRUE(std::equal(visited.begin(), visited.end(), expected.begin()));
  }

  buffer.clear();

  for (auto&& ele : buffer) {
    // This will never be called.
    EXPECT_EQ(true, false) << "Unexpected phase: " << ele.first;
  }

  buffer.resize(0);

  for (auto&& ele : buffer) {
    // This will never be called.
    EXPECT_EQ(true, false) << "Unexpected phase: " << ele.first;
  }
}

}}} /* end namespace vt::tests::unit */
