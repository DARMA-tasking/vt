/*
//@HEADER
// *****************************************************************************
//
//                          test_trace_spec_reader.cc
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

#include <vt/transport.h>
#include <vt/trace/file_spec/spec.h>

#include "test_parallel_harness.h"

#include <fstream>

namespace vt { namespace tests { namespace unit {

using TestTraceSpec = TestParallelHarness;

TEST_F(TestTraceSpec, test_trace_spec_1) {
  using Spec = vt::trace::file_spec::TraceSpec;

  std::string file_name = "test_trace_spec_1.txt";
  if (theContext()->getNode() == 0) {
    std::ofstream out(file_name);
    out << ""
      "0 0 3\n"
      "%5 -1 1\n";
    out.close();
  }
  theCollective()->barrier();

  theArgConfig()->vt_trace_spec = true;
  theArgConfig()->vt_trace_spec_file = file_name;

  auto proxy = Spec::construct();
  theTerm()->produce();
  if (theContext()->getNode() == 0) {
    proxy.get()->parse();
    proxy.get()->broadcastSpec();
  }
  do vt::runScheduler(); while (not proxy.get()->specReceived());
  theTerm()->consume();

  for (typename Spec::SpecIndex i = 0; i < 1000; i++) {
    bool is_enabled = proxy.get()->checkTraceEnabled(i);
    EXPECT_EQ(is_enabled, i < 4 or i % 5 == 0 or i % 5 == 1 or i % 5 == 4);
  }
}

TEST_F(TestTraceSpec, test_trace_spec_2) {
  using Spec = vt::trace::file_spec::TraceSpec;

  std::string file_name = "test_trace_spec_2.txt";
  if (theContext()->getNode() == 0) {
    std::ofstream out(file_name);
    out << ""
      "%   10 0     1   \n"
      "0   -100    0\n"
      "\n"
      "\n"
      ;
    out.close();
  }
  theCollective()->barrier();

  theArgConfig()->vt_trace_spec = true;
  theArgConfig()->vt_trace_spec_file = file_name;

  auto proxy = Spec::construct();
  theTerm()->produce();
  if (theContext()->getNode() == 0) {
    proxy.get()->parse();
    proxy.get()->broadcastSpec();
  }
  do vt::runScheduler(); while (not proxy.get()->specReceived());
  theTerm()->consume();

  for (typename Spec::SpecIndex i = 0; i < 1000; i++) {
    bool is_enabled = proxy.get()->checkTraceEnabled(i);
    EXPECT_EQ(is_enabled, i % 10 == 0 or i % 10 == 1);
  }
}

TEST_F(TestTraceSpec, test_trace_spec_3) {
  using Spec = vt::trace::file_spec::TraceSpec;

  std::string file_name = "test_trace_spec_3.txt";
  if (theContext()->getNode() == 0) {
    std::ofstream out(file_name);
    out << ""
      "%   10 -5 0   \n"
      "0   0    1\n"
      "%   20 -3 3   \n"
      "   100 -10 10   \n"
      "\n"
      "\n"
      ;
    out.close();
  }
  theCollective()->barrier();

  theArgConfig()->vt_trace_spec = true;
  theArgConfig()->vt_trace_spec_file = file_name;

  auto proxy = Spec::construct();
  theTerm()->produce();
  if (theContext()->getNode() == 0) {
    proxy.get()->parse();
    proxy.get()->broadcastSpec();
  }
  do vt::runScheduler(); while (not proxy.get()->specReceived());
  theTerm()->consume();

  for (typename Spec::SpecIndex i = 0; i < 1000; i++) {
    bool is_enabled = proxy.get()->checkTraceEnabled(i);
    EXPECT_EQ(
      is_enabled,
      i == 0 or i == 1 or
      i % 20 <= 3 or
      i % 10 >= 5 or i % 10 == 0 or
      (i >= 90 and i <= 110)
    );
  }
}

TEST_F(TestTraceSpec, test_trace_spec_4) {
  using Spec = vt::trace::file_spec::TraceSpec;

  std::string file_name = "test_trace_spec_4.txt";
  if (theContext()->getNode() == 0) {
    std::ofstream out(file_name);
    out << ""
      "% 98   -5 0  \n"
      "  %   99 -3 3   \n"
      "   1 0 2   \n"
      "\n"
      "\n"
      ;
    out.close();
  }
  theCollective()->barrier();

  theArgConfig()->vt_trace_spec = true;
  theArgConfig()->vt_trace_spec_file = file_name;

  auto proxy = Spec::construct();
  theTerm()->produce();
  if (theContext()->getNode() == 0) {
    proxy.get()->parse();
    proxy.get()->broadcastSpec();
  }
  do vt::runScheduler(); while (not proxy.get()->specReceived());
  theTerm()->consume();

  for (typename Spec::SpecIndex i = 0; i < 1000; i++) {
    bool is_enabled = proxy.get()->checkTraceEnabled(i);
    EXPECT_EQ(
      is_enabled,
      i == 1 or i == 2 or i == 3 or
      i % 99 >= 96 or i % 99 <= 3 or
      i % 98 >= 93 or i % 98 == 0
    );
  }
}

}}} // end namespace vt::tests::unit
