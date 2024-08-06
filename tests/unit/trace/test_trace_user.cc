/*
//@HEADER
// *****************************************************************************
//
//                              test_trace_user.cc
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

#include "test_parallel_harness.h"
#include "test_helpers.h"

#if vt_check_enabled(trace_enabled)

#include <vt/trace/trace_user_event.h>
#include <vt/trace/trace_constants.h>

#include <zlib.h>
#include <filesystem>

namespace vt { namespace tests { namespace unit {

struct TestTraceUser : TestParallelHarness {
  // Set dir for tracing to be the same as test name
  void addAdditionalArgs() override {
    trace_dir = fmt::format(
      "--vt_trace_dir={}",
      ::testing::UnitTest::GetInstance()->current_test_info()->name());
    addArgs(trace_dir);
  }

  virtual void TearDown() override {
    TestHarnessAny<testing::Test>::TearDown();
  }

  static void stopVt() {
    try {
      vt::theSched()->runSchedulerWhile([] { return !rt->isTerminated(); });
    } catch (std::exception& e) {
      ADD_FAILURE() << fmt::format("Caught an exception: {}\n", e.what());
    }

#if DEBUG_TEST_HARNESS_PRINT
    auto const& my_node = theContext()->getNode();
    fmt::print("my_node={}, tearing down runtime\n", my_node);
#endif

    CollectiveOps::finalize();
  }

private:
  std::string trace_dir;
};

void validateLine(const std::string line, int& time) {
  using vt::trace::eTraceConstants;

  // skip header (starts with P)
  if (line.front() == 80) {
    return;
  }
  // Split to tokens
  std::stringstream stream(line);
  std::vector<std::string> tokens;
  std::string token;
  while (stream >> token) {
    tokens.push_back(token);
  }

  // validate the start time
  int start_time = 0, end_time = 0;
  bool check_start_time = true, check_end_time = false;
  eTraceConstants type = static_cast<eTraceConstants>(std::stoi(tokens[0]));
  switch(type) {
    case eTraceConstants::Creation: // 1
    case eTraceConstants::BeginProcessing: // 2
    case eTraceConstants::EndProcessing: // 3
    case eTraceConstants::CreationBcast: // 20
      start_time = std::stoi(tokens[3]);
      break;
    case eTraceConstants::UserEvent: // 13
    case eTraceConstants::UserSupplied: // 26
    case eTraceConstants::BeginUserEventPair: // 98
    case eTraceConstants::EndUserEventPair: // 99
      start_time = std::stoi(tokens[2]);
      break;
    case eTraceConstants::BeginComputation: // 6
    case eTraceConstants::EndComputation: // 7
    case eTraceConstants::BeginIdle: // 14
    case eTraceConstants::EndIdle: // 15
    case eTraceConstants::UserSuppliedNote: // 28
      start_time = std::stoi(tokens[1]);
      break;
    // those with end time
    case eTraceConstants::UserSuppliedBracketedNote: // 29
      start_time = std::stoi(tokens[1]);
      end_time = std::stoi(tokens[2]);
      check_end_time = true;
      break;
    default:
      check_start_time = false;
  }

  if (check_start_time) {
    EXPECT_GE(start_time, time);
    time = start_time;
  }
  if (check_end_time) {
    EXPECT_GE(end_time, start_time);
  }
}

void validateAllTraceFiles() {
  std::string test_name =
    ::testing::UnitTest::GetInstance()->current_test_info()->name();
  std::filesystem::path path = std::filesystem::current_path() / test_name / "";
  std::string ext = ".gz";

  // iterate over files in test output directory
  for (const auto& file : std::filesystem::directory_iterator(path)) {
    if (file.path().extension() == ext) {
      auto log_file_ =
        std::make_shared<gzFile>(gzopen(file.path().c_str(), "rb"));
      EXPECT_TRUE(nullptr != log_file_);

      // should be more than enough
      char buffer[4096];
      int bytesRead = gzread(*log_file_, buffer, sizeof(buffer));
      EXPECT_GT(bytesRead, 0);

      std::istringstream stream(std::string(buffer, bytesRead));

      int lastStartTime = 0;
      std::string line;
      // read line by line all validate start time
      while (std::getline(stream, line)) {
        validateLine(line, lastStartTime);
      }
    }
  }
}

TEST_F(TestTraceUser, trace_user_add_note_pre_epi) {
  if (!theTrace()->checkDynamicRuntimeEnabled()) {
    TestTraceUser::stopVt();
    GTEST_SKIP() << "trace tests require --vt_trace to be set";
  }

  // Only the begging of the note
  {
    trace::addUserNotePre("some note", 2);
  }
  // Two opening notes with the same id
  {
    trace::addUserNotePre("note 1", 5);
    trace::addUserNotePre("note 2", 5);
  }
  // note with events in between
  {
    trace::addUserEvent(901);
    trace::addUserNotePre("TEST NOTE", 10);
    trace::addUserEvent(902);
    trace::addUserNoteEpi("TEST NOTE", 10);
    trace::addUserEvent(903);
  }

  TestTraceUser::stopVt();
  validateAllTraceFiles();
}

TEST_F(TestTraceUser, trace_user_scoped_event_hash) {
  if (!theTrace()->checkDynamicRuntimeEnabled()) {
    TestTraceUser::stopVt();
    GTEST_SKIP() << "trace tests require --vt_trace to be set";
  }

  // single event
  {
    trace::TraceScopedEventHash("test_event");
  }
  // a few events
  {
    auto ev1 = trace::TraceScopedEventHash("test_event_1");
    auto ev2 = trace::TraceScopedEventHash("test_event_2");
    auto ev3 = trace::TraceScopedEventHash("test_event_3");
  }
  // nested events
  {
    auto ev1 = trace::TraceScopedEventHash("test_event_1");
    {
        auto ev2 = trace::TraceScopedEventHash("test_event_2");
        {
            auto ev3 = trace::TraceScopedEventHash("test_event_3");
        }
    }
  }

  TestTraceUser::stopVt();
  validateAllTraceFiles();
}

TEST_F(TestTraceUser, trace_user_scoped_event) {
  if (!theTrace()->checkDynamicRuntimeEnabled()) {
    TestTraceUser::stopVt();
    GTEST_SKIP() << "trace tests require --vt_trace to be set";
  }

  // single event
  {
    trace::TraceScopedEvent(123);
  }
  // a few events
  {
    auto ev1 = trace::TraceScopedEvent(1);
    auto ev2 = trace::TraceScopedEvent(2);
    auto ev3 = trace::TraceScopedEvent(3);
  }
  // nested
  {
    auto ev1 = trace::TraceScopedEvent(123);
    {
        auto ev2 = trace::TraceScopedEvent(123);
        {
            auto ev3 = trace::TraceScopedEvent(123);
        }
    }
  }

  TestTraceUser::stopVt();
  validateAllTraceFiles();
}

TEST_F(TestTraceUser, trace_user_scoped_note) {
  if (!theTrace()->checkDynamicRuntimeEnabled()) {
    TestTraceUser::stopVt();
    GTEST_SKIP() << "trace tests require --vt_trace to be set";
  }

  // single note
  {
    trace::TraceScopedNote(123);
    trace::TraceScopedNote("Some note", 456);

    auto note1 = trace::TraceScopedNote(1);
    auto note2 = trace::TraceScopedNote("note", 2);
    auto note3 = trace::TraceScopedNote(3);
  }
  // nested
  {
    auto note11 = trace::TraceScopedNote(123);
    auto note12 = trace::TraceScopedNote("test", 456);
    {
        auto note21 = trace::TraceScopedNote(123);
        auto note22 = trace::TraceScopedNote("test 2", 456);
        {
            auto note31 = trace::TraceScopedNote(123);
            auto note32 = trace::TraceScopedNote("test 3", 456);
        }
    }
  }

  TestTraceUser::stopVt();
  validateAllTraceFiles();
}

TEST_F(TestTraceUser, trace_user_note_bracketed) {
  if (!theTrace()->checkDynamicRuntimeEnabled()) {
    TestTraceUser::stopVt();
    GTEST_SKIP() << "trace tests require --vt_trace to be set";
  }

  theTrace()->addUserNoteBracketedBeginTime("OUTER TEST NOTE 10", 10);
  theTrace()->addUserNoteBracketedBeginTime("INNER TEST NOTE 10", 10);
  theTrace()->addUserNoteBracketedBeginTime("INNER INNER TEST NOTE 10", 10);
  theTrace()->addUserNoteBracketedBeginTime("INNER INNER INNER TEST NOTE 12", 12);

  theTrace()->addUserEvent(901);
  trace::addUserNote("Note 0");
  theTrace()->addUserEvent(902);

  theTrace()->addUserNoteBracketedEndTime(12);
  trace::addUserNote("Note 1");
  theTrace()->addUserNoteBracketedEndTime(10);
  trace::addUserNote("Note 2");
  theTrace()->addUserNoteBracketedEndTime(10);
  trace::addUserNote("Note 3");
  theTrace()->addUserNoteBracketedEndTime(10);
  trace::addUserNote("Note 4");

  TestTraceUser::stopVt();
  validateAllTraceFiles();
}

TEST_F(TestTraceUser, trace_user_note_bracketed_override_note) {
  if (!theTrace()->checkDynamicRuntimeEnabled()) {
    TestTraceUser::stopVt();
    GTEST_SKIP() << "trace tests require --vt_trace to be set";
  }

  theTrace()->addUserNoteBracketedBeginTime("ABC", 10);
  theTrace()->addUserNoteBracketedBeginTime("123", 10);
  theTrace()->addUserNoteBracketedBeginTime("X", 10);
  theTrace()->addUserNoteBracketedBeginTime("", 12);

  theTrace()->addUserEvent(901);
  trace::addUserNote("Note 0");
  theTrace()->addUserEvent(902);

  theTrace()->addUserNoteBracketedEndTime(12, "INNER INNER INNER TEST NOTE 12");
  trace::addUserNote("Note 1");
  theTrace()->addUserNoteBracketedEndTime(10, "INNER INNER TEST NOTE 10");
  trace::addUserNote("Note 2");
  theTrace()->addUserNoteBracketedEndTime(10, "INNER TEST NOTE 10");
  trace::addUserNote("Note 3");
  theTrace()->addUserNoteBracketedEndTime(10, "OUTER TEST NOTE 10");
  trace::addUserNote("Note 4");

  TestTraceUser::stopVt();
  validateAllTraceFiles();
}

TEST_F(TestTraceUser, trace_user_bracketed_event) {
  if (!theTrace()->checkDynamicRuntimeEnabled()) {
    TestTraceUser::stopVt();
    GTEST_SKIP() << "trace tests require --vt_trace to be set";
  }

  theTrace()->addUserEvent(124);
  theTrace()->addUserNote("Some Note");

  theTrace()->addUserEventBracketedBegin(1);
  theTrace()->addUserEventBracketedBegin(2);
  theTrace()->addUserEventBracketedEnd(2);
  theTrace()->addUserEventBracketedBegin(3);

  theTrace()->addUserEventManual(123);
  theTrace()->addUserData(123456789);

  theTrace()->addUserEventBracketedEnd(1);
  theTrace()->addUserEventBracketedEnd(3);

  theTrace()->addUserNote("Some Note 2");
  theTrace()->addUserData(123456);
  theTrace()->addUserEvent(124);

  TestTraceUser::stopVt();
  validateAllTraceFiles();
}

}}} // end namespace vt::tests::unit

#endif // vt_check_enabled(trace_enabled)
