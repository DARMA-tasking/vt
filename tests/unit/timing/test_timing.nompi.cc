/*
//@HEADER
// *****************************************************************************
//
//                             test_timing.nompi.cc
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

#include "test_harness.h"

#include "vt/timing/timing.h"

namespace vt { namespace tests { namespace unit {

using TestTiming = TestHarness;

TEST_F(TestTiming, test_time_formatting) {
  {
    TimeType const t = TimeType{15664645.400691890716};
    EXPECT_EQ(fmt::format("{}", t), "15.665e6 s");
  }

  {
    TimeType const t = TimeType{4.0006918907165527344};
    EXPECT_EQ(fmt::format("{}", t), "4.0007e0 s");
  }

  {
    TimeType const t = TimeType{0.0691890716552734423};
    EXPECT_EQ(fmt::format("{}", t), "69.189e-3 s");
  }

  {
    TimeType const t = TimeType{-0.0691890716552734423};
    EXPECT_EQ(fmt::format("{}", t), "-69.189e-3 s");
  }

  {
    TimeType const t = TimeType{0.0006918907165527344};
    EXPECT_EQ(fmt::format("{}", t), "691.89e-6 s");
  }

  {
    TimeType const t = TimeType{0.0000006918907165527};
    EXPECT_EQ(fmt::format("{}", t), "691.89e-9 s");
  }

  {
    TimeType const t = TimeType{3.14};
    EXPECT_EQ(fmt::format("{}", t), "3.1400e0 s");
  }
}

}}} /* end namespace vt::tests::unit */
