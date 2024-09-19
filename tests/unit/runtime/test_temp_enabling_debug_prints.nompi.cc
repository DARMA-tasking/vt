/*
//@HEADER
// *****************************************************************************
//
//                   test_temp_enabling_debug_prints.nompi.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#include "test_harness.h"

#include <vt/collective/startup.h>
#include <vt/configs/arguments/app_config.h>

namespace vt { namespace tests { namespace unit {

struct TestTempEnablingDisablingDebugPrints : TestHarness { };

TEST_F(TestTempEnablingDisablingDebugPrints, test_manual_enabling_disabling) {
  vt::initialize();

  EXPECT_EQ(theConfig()->vt_debug_all, false);
  EXPECT_EQ(theConfig()->vt_debug_termds, false);
  EXPECT_EQ(theConfig()->vt_debug_param, false);
  EXPECT_EQ(theConfig()->vt_debug_scatter, false);

  vt_debug_temp_enable_opt(all);
  vt_debug_temp_enable_opt(termds);
  vt_debug_temp_enable_opt(param);
  vt_debug_temp_enable_opt(scatter);

  EXPECT_EQ(theConfig()->vt_debug_all, true);
  EXPECT_EQ(theConfig()->vt_debug_termds, true);
  EXPECT_EQ(theConfig()->vt_debug_param, true);
  EXPECT_EQ(theConfig()->vt_debug_scatter, true);

  vt_debug_temp_disable_opt(all);
  vt_debug_temp_disable_opt(termds);
  vt_debug_temp_disable_opt(param);
  vt_debug_temp_disable_opt(scatter);

  EXPECT_EQ(theConfig()->vt_debug_all, false);
  EXPECT_EQ(theConfig()->vt_debug_termds, false);
  EXPECT_EQ(theConfig()->vt_debug_param, false);
  EXPECT_EQ(theConfig()->vt_debug_scatter, false);

  vt::finalize();
}

TEST_F(TestTempEnablingDisablingDebugPrints, test_scoped_enabling_disabling) {
  vt::initialize();

  EXPECT_EQ(theConfig()->vt_debug_none, false);
  EXPECT_EQ(theConfig()->vt_debug_active, false);
  EXPECT_EQ(theConfig()->vt_debug_reduce, false);
  EXPECT_EQ(theConfig()->vt_debug_context, false);

  {
    auto const scopedEnabledNone = vt_debug_scoped_enable_opt(none);
    auto const scopedEnabledActive = vt_debug_scoped_enable_opt(active);
    auto const scopedEnabledReduce = vt_debug_scoped_enable_opt(reduce);
    auto const scopedEnabledContext = vt_debug_scoped_enable_opt(context);

    EXPECT_EQ(theConfig()->vt_debug_none, true);
    EXPECT_EQ(theConfig()->vt_debug_active, true);
    EXPECT_EQ(theConfig()->vt_debug_reduce, true);
    EXPECT_EQ(theConfig()->vt_debug_context, true);
  }

  EXPECT_EQ(theConfig()->vt_debug_none, false);
  EXPECT_EQ(theConfig()->vt_debug_active, false);
  EXPECT_EQ(theConfig()->vt_debug_reduce, false);
  EXPECT_EQ(theConfig()->vt_debug_context, false);

  vt::finalize();
}

}}} // end namespace vt::tests::unit
