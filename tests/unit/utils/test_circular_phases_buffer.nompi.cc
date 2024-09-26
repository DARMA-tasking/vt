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
  PhaseType x;
};

using CircularBufferType = util::container::CircularPhasesBuffer<Dummy>;

void validatePresentPhases(CircularBufferType& buffer, std::vector<PhaseType> expected) {
  for (auto&& phase : expected) {
    EXPECT_TRUE(buffer.contains(phase)) << "   Phase: " << phase;
    EXPECT_EQ(phase, buffer.at(phase).x) << "   Phase: " << phase;
    EXPECT_EQ(phase, buffer[phase].x) << "   Phase: " << phase;
    EXPECT_EQ(phase, buffer.find(phase)->x) << "   Phase: " << phase;
  }
}

void validateMissingPhases(CircularBufferType& buffer, std::vector<PhaseType> expected) {
  for (auto&& phase : expected) {
    EXPECT_FALSE(buffer.contains(phase)) << "   Phase: " << phase;
    EXPECT_EQ(nullptr, buffer.find(phase)) << "   Phase: " << phase;
  }
}

TEST_F(TestCircularPhasesBuffer, test_circular_phases_buffer_empty) {
  CircularBufferType buffer;

  EXPECT_FALSE(buffer.contains(0));
  EXPECT_EQ(std::size_t{0}, buffer.size());
  EXPECT_TRUE(buffer.empty());
  EXPECT_EQ(std::numeric_limits<PhaseType>::max(), buffer.frontPhase());
  EXPECT_EQ(nullptr, buffer.find(0));

  buffer.resize(2);

  EXPECT_FALSE(buffer.contains(0));
  EXPECT_EQ(std::size_t{0}, buffer.size());
  EXPECT_TRUE(buffer.empty());
  EXPECT_EQ(std::numeric_limits<PhaseType>::max(), buffer.frontPhase());
  EXPECT_EQ(nullptr, buffer.find(0));

  buffer.clear();

  EXPECT_FALSE(buffer.contains(1));
  EXPECT_EQ(std::size_t{0}, buffer.size());
  EXPECT_TRUE(buffer.empty());
  EXPECT_EQ(std::numeric_limits<PhaseType>::max(), buffer.frontPhase());
  EXPECT_EQ(nullptr, buffer.find(0));

  buffer.resize(4);
  buffer[0] = {0};

  EXPECT_TRUE(buffer.contains(0));
  EXPECT_EQ(std::size_t{1}, buffer.size());
  EXPECT_FALSE(buffer.empty());
  EXPECT_EQ(PhaseType{0}, buffer.frontPhase());
  EXPECT_EQ(PhaseType{0}, buffer.frontData().x);
  EXPECT_NE(nullptr, buffer.find(0));

  buffer[1] = {1};
  buffer[2] = {4};
  buffer[3] = {6};
  buffer[4] = {8};

  EXPECT_TRUE(buffer.contains(1));
  EXPECT_EQ(std::size_t{4}, buffer.size());
  EXPECT_FALSE(buffer.empty());
  EXPECT_EQ(PhaseType{4}, buffer.frontPhase());
  EXPECT_EQ(PhaseType{8}, buffer.frontData().x);
  EXPECT_EQ(nullptr, buffer.find(0));
  EXPECT_NE(nullptr, buffer.find(3));

  buffer.clear();

  EXPECT_FALSE(buffer.contains(0));
  EXPECT_EQ(std::size_t{0}, buffer.size());
  EXPECT_TRUE(buffer.empty());
  EXPECT_EQ(std::numeric_limits<PhaseType>::max(), buffer.frontPhase());
  EXPECT_EQ(nullptr, buffer.find(0));
}

TEST_F(TestCircularPhasesBuffer, test_circular_phases_buffer_single_element) {
  CircularBufferType buffer{1};

  EXPECT_EQ(false, buffer.contains(0));

  for (PhaseType i = 0; i < 5; i++) {
    buffer.store(i, {i});
    EXPECT_EQ(i == 0, buffer.contains(0));
    EXPECT_EQ(i == 1, buffer.contains(1));
    EXPECT_EQ(i == 2, buffer.contains(2));
    EXPECT_EQ(i == 3, buffer.contains(3));
    EXPECT_EQ(i == 4, buffer.contains(4));
    EXPECT_EQ(i == 5, buffer.contains(5));
    EXPECT_EQ(i == 6, buffer.contains(6));

    EXPECT_EQ(i, buffer.frontPhase());
    EXPECT_EQ(i, buffer.frontData().x);
    EXPECT_NE(nullptr, buffer.find(i));
    EXPECT_NO_THROW(buffer.at(i));
  }
}

TEST_F(TestCircularPhasesBuffer, test_circular_phases_buffer_multi_elements) {
  CircularBufferType buffer{5};

  for (PhaseType i = 0; i < 15; i++) {
    buffer.store(i, {i});

    EXPECT_EQ(i, buffer.frontPhase());
    EXPECT_EQ(i, buffer.frontData().x);
    EXPECT_NE(nullptr, buffer.find(i));
    EXPECT_NO_THROW(buffer.at(i));
  }
  std::vector<PhaseType> finalOutput = {10, 11, 12, 13, 14};
  validatePresentPhases(buffer, finalOutput);
}

TEST_F(TestCircularPhasesBuffer, test_circular_phases_buffer_store) {
  CircularBufferType buffer;

  buffer.store(2, {2});
  validatePresentPhases(buffer, {2});
  validateMissingPhases(buffer, {0, 1});

  buffer.resize(10);

  validatePresentPhases(buffer, {2});
  validateMissingPhases(buffer, {0, 1, 3, 4, 5, 6, 7, 8, 9});

  // store series of elements
  for (PhaseType i = 3; i < 15; i++) {
    buffer.store(i, {i});
  }

  validateMissingPhases(buffer, {0, 1, 2, 3, 4});
  std::vector<PhaseType> finalOutput = {5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
  validatePresentPhases(buffer, finalOutput);
}

TEST_F(TestCircularPhasesBuffer, test_circular_phases_buffer_resize) {
  CircularBufferType buffer{10};

  for (PhaseType i = 0; i <= 15; i++) {
    buffer[i] = {i};
  }
  validatePresentPhases(buffer, {6, 7, 8, 9, 10, 11, 12, 13, 14, 15});

  buffer.resize(5);
  validatePresentPhases(buffer, {11, 12, 13, 14, 15});
  validateMissingPhases(buffer, {6, 7, 8, 9, 10});

  for (PhaseType i = 15; i <= 32; i++) {
    buffer[i] = {i};
  }
  validatePresentPhases(buffer, {28, 29, 30, 31, 32});
  validateMissingPhases(buffer, {11, 12, 13, 14, 15});

  buffer.resize(9);

  validatePresentPhases(buffer, {28, 29, 30, 31, 32});

  for (PhaseType i = 33; i <= 35; i++) {
    buffer[i] = {i};
  }
  validatePresentPhases(buffer, {28, 29, 30, 31, 32, 33, 34, 35});

  buffer.resize(1);
  validatePresentPhases(buffer, {35});
  validateMissingPhases(buffer, {28, 29, 30, 31, 32, 33, 34});
}

TEST_F(TestCircularPhasesBuffer, test_circular_phases_buffer_restart_from) {
  CircularBufferType buffer{1};

  buffer.restartFrom(5);
  EXPECT_EQ(PhaseType{5}, buffer.frontPhase());

  buffer[5] = {5};
  EXPECT_EQ(PhaseType{5}, buffer.frontPhase());
  EXPECT_EQ(PhaseType{5}, buffer.frontData().x);

  buffer.restartFrom(10);
  EXPECT_EQ(PhaseType{10}, buffer.frontPhase());
  EXPECT_EQ(PhaseType{5}, buffer.frontData().x);

  buffer.restartFrom(0);
  EXPECT_EQ(PhaseType{0}, buffer.frontPhase());
  EXPECT_EQ(PhaseType{5}, buffer.frontData().x);
}

TEST_F(TestCircularPhasesBuffer, test_circular_phases_buffer_forward_iter) {
  CircularBufferType buffer;

  for (auto& ele: buffer) {
    EXPECT_EQ(true, false) << "   Phase: " << ele.first;
  }
  for (auto&& ele : buffer) {
    EXPECT_EQ(true, false) << "   Phase: " << ele.first;
  }

  buffer.resize(9);
  buffer[0] = {0};
  buffer[3] = {3};
  buffer[7] = {7};

  {
    std::vector<PhaseType> visited;
    for (auto&& ele : buffer) {
      visited.push_back(ele.first);
    }
    validatePresentPhases(buffer, visited);
    EXPECT_EQ(visited.size(), buffer.size());
  }

  {
    std::vector<PhaseType> visited;
    for (auto& ele : buffer) {
      visited.push_back(ele.first);
    }
    validatePresentPhases(buffer, visited);
    EXPECT_EQ(visited.size(), buffer.size());
  }

  for (PhaseType i = 0; i <= 15; i++) {
    buffer[i] = {i};
  }

  {
    std::vector<PhaseType> visited;
    for (auto&& ele : buffer) {
      visited.push_back(ele.first);
    }
    validatePresentPhases(buffer, visited);
    EXPECT_EQ(visited.size(), buffer.size());
  }

  {
    std::vector<PhaseType> visited;
    for (auto& ele : buffer) {
      visited.push_back(ele.first);
    }
    validatePresentPhases(buffer, visited);
    EXPECT_EQ(visited.size(), buffer.size());
  }
}

}}} /* end namespace vt::tests::unit */
