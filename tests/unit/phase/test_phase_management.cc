/*
//@HEADER
// *****************************************************************************
//
//                          test_phase_management.cc
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

#include "test_parallel_harness.h"

#include <vt/transport.h>

namespace vt { namespace tests { namespace unit {

using TestPhaseManagement = TestParallelHarness;

TEST_F(TestPhaseManagement, test_phase_manager_1) {
  auto phase_mgr = phase::PhaseManager::construct();

  // start with phase 0
  EXPECT_EQ(phase_mgr->getCurrentPhase(), 0);

  int start_hooks = 0;
  int end_hooks = 0;
  int end_post_hooks = 0;

  auto start = [&]{ start_hooks++; };
  auto end = [&]{ end_hooks++; };
  auto endpost = [&]{ end_post_hooks++; };

  // register a starting hook
  auto hookid = phase_mgr->registerHookRooted(phase::PhaseHook::Start, start);

  // unregister it, make sure it doesn't run
  phase_mgr->unregisterHook(hookid);

  // run startup function, which will trigger starting hooks
  phase_mgr->startup();
  EXPECT_EQ(start_hooks, 0);

  auto hookid2 = phase_mgr->registerHookRooted(phase::PhaseHook::Start, start);

  // run startup function, which will trigger starting hooks
  phase_mgr->startup();
  EXPECT_EQ(start_hooks, 1);

  auto hookid3 = phase_mgr->registerHookCollective(phase::PhaseHook::Start, start);
  phase_mgr->unregisterHook(hookid3);

  // run startup function, which will trigger starting hooks
  phase_mgr->startup();
  EXPECT_EQ(start_hooks, 2);
  phase_mgr->unregisterHook(hookid2);

  phase_mgr->registerHookCollective(phase::PhaseHook::Start, start);
  phase_mgr->registerHookCollective(phase::PhaseHook::End, end);
  phase_mgr->registerHookCollective(phase::PhaseHook::EndPostMigration, endpost);

  phase_mgr->nextPhaseCollective();
  EXPECT_EQ(start_hooks, 3);
  EXPECT_EQ(end_hooks, 1);
  EXPECT_EQ(end_post_hooks, 1);
  EXPECT_EQ(phase_mgr->getCurrentPhase(), 1);

  phase_mgr->nextPhaseCollective();
  EXPECT_EQ(start_hooks, 4);
  EXPECT_EQ(end_hooks, 2);
  EXPECT_EQ(end_post_hooks, 2);
  EXPECT_EQ(phase_mgr->getCurrentPhase(), 2);
}

}}} // end namespace vt::tests::unit
