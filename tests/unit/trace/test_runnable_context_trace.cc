/*
//@HEADER
// *****************************************************************************
//
//                        test_runnable_context_trace.cc
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

#include <vt/context/runnable_context/trace.h>

#include "test_parallel_harness.h"
#include "test_helpers.h"

#if vt_check_enabled(trace_enabled)

namespace vt { namespace tests { namespace unit {

using TestRunnableContextTrace = TestParallelHarness;

struct TraceMsg : ::vt::Message {
};

void handler_func( TraceMsg * )
{}

TEST_F(TestRunnableContextTrace, runnable_context_trace_test_1) {
  if (!theTrace()->checkDynamicRuntimeEnabled())
    GTEST_SKIP() << "trace tests require --vt_trace to be set";

  auto msg = makeMessage< TraceMsg >();

  auto handler = auto_registry::makeAutoHandler< TraceMsg, handler_func >();

  HandlerManager::setHandlerTrace(handler, true);

  // Give some nonsense parameters but Trace shouldn't touch them
  auto t = ctx::Trace( msg, /* in_trace_event */ 7,
                  handler, /* in_from_node */ 3,
                  5, 9, 3, 2 );

  // One event for the trace, one for the top open event, third if mem usage tracing is enabled
  const int add_num_events = theConfig()->vt_trace_memory_usage ? 3 : 2;

  auto num_events = theTrace()->getNumTraceEvents();
  t.start(TimeType{3});
  EXPECT_EQ(num_events + add_num_events, theTrace()->getNumTraceEvents());
  auto *last_trace = theTrace()->getLastTraceEvent();
  EXPECT_NE(last_trace, nullptr);
  EXPECT_EQ(last_trace->type, theConfig()->vt_trace_memory_usage ? trace::eTraceConstants::MemoryUsageCurrent : trace::eTraceConstants::BeginProcessing);
  EXPECT_EQ(last_trace->time, TimeType{3});

  num_events = theTrace()->getNumTraceEvents();
  t.finish(TimeType{7});
  EXPECT_EQ(num_events + add_num_events, theTrace()->getNumTraceEvents());
  last_trace = theTrace()->getLastTraceEvent();
  EXPECT_NE(last_trace, nullptr);
  // Counterintuitive, but we restart the open event as the last action
  EXPECT_EQ(last_trace->type, trace::eTraceConstants::BeginProcessing);
  EXPECT_EQ(last_trace->time, TimeType{7}); // Time should still match though
}

}}} // end namespace vt::tests::unit

#endif // vt_check_enabled(trace_enabled)
