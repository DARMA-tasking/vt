/*
//@HEADER
// *****************************************************************************
//
//                         test_memory_footprinting.cc
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2020 National Technology & Engineering Solutions of Sandia, LLC
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

#include <checkpoint/checkpoint.h>
#include "vt/configs/arguments/args.h"
#include "vt/timetrigger/time_trigger_manager.h"
#include "vt/vrt/collection/balance/lb_invoke/lb_manager.h"
#include "vt/vrt/collection/balance/stats_restart_reader.h"

#include "test_harness.h"

namespace vt { namespace tests { namespace unit {

using TestMemoryFootprinting = TestHarness;

template<typename T>
void printMemoryFootprint(T& obj) {
  fmt::print("sizeof(obj):\t{}\n", sizeof(obj));

  auto size = checkpoint::getMemoryFootprint<T>(obj);
  fmt::print("footprint(obj):\t{}\n", size);
}

TEST_F(TestMemoryFootprinting, test_arg_config) {
  arguments::ArgConfig args;
  printMemoryFootprint(args);
}

TEST_F(TestMemoryFootprinting, test_time_trigger_manager) {
  timetrigger::TimeTriggerManager trigger_manager;
  printMemoryFootprint(trigger_manager);
}

TEST_F(TestMemoryFootprinting, test_lb_manager) {
  vt::vrt::collection::balance::LBManager lb_manager;
  printMemoryFootprint(lb_manager);
}

TEST_F(TestMemoryFootprinting, test_stats_restart_reader) {
  vt::vrt::collection::balance::StatsRestartReader reader;
  printMemoryFootprint(reader);
}

}}} /* end namespace vt::tests::unit */
