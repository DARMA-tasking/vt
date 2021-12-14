/*
//@HEADER
// *****************************************************************************
//
//                                test_timing.cc
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

#include "test_parallel_harness.h"

#include "vt/configs/arguments/app_config.h"
#include "vt/timing/timing.h"

namespace vt { namespace tests { namespace unit {

using TestTiming = TestParallelHarness;

TEST_F(TestTiming, test_time_formatting) {
  theConfig()->vt_time_units = true;

  {
    auto const t = 15664645.400691890716;
    EXPECT_EQ(timing::getTimeWithUnits(t), "15664645.4 sec");
  }

  {
    auto const t = 4.0006918907165527344;
    EXPECT_EQ(timing::getTimeWithUnits(t), "4.0 sec");
  }

  {
    auto const t = 0.0691890716552734423;
    EXPECT_EQ(timing::getTimeWithUnits(t), "69.2 ms");
  }

  {
    auto const t = -0.0691890716552734423;
    EXPECT_EQ(timing::getTimeWithUnits(t), "-69.2 ms");
  }

  {
    auto const t = 0.0006918907165527344;
    EXPECT_EQ(timing::getTimeWithUnits(t), "691.9 μs");
  }

  {
    auto const t = 0.0000006918907165527;
    EXPECT_EQ(timing::getTimeWithUnits(t), "691.9 ns");
  }
}

}}} /* end namespace vt::tests::unit */
