/*
//@HEADER
// *****************************************************************************
//
//                        test_lbargs_enum_conv.nompi.cc
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

#include "vt/vrt/collection/balance/lb_args_enum_converter.h"
#include "vt/vrt/collection/balance/read_lb.h"

#include "test_harness.h"

namespace vt { namespace tests { namespace unit {

using TestLBArgsEnumConverter = TestHarness;

enum struct DummyEnum : uint8_t {
  One = 1,
  Two = 2,
  Three = 3
};

template <typename E>
void checkEnum(vrt::collection::balance::LBArgsEnumConverter<E> &conv, E e) {
  EXPECT_EQ(conv.getEnum(conv.getString(e)), e);
}

template <typename E>
void checkString(
  vrt::collection::balance::LBArgsEnumConverter<E> &conv, const std::string &s
) {
  EXPECT_EQ(conv.getString(conv.getEnum(s)).compare(s), 0);
}

TEST_F(TestLBArgsEnumConverter, test_enum_converter_mapping) {
  vrt::collection::balance::LBArgsEnumConverter<DummyEnum> dummy_(
    "dummy", "DummyEnum", {
      {DummyEnum::One,   "One"},
      {DummyEnum::Two,   "Two"},
      {DummyEnum::Three, "Three"}
    }
  );

  checkEnum(dummy_, DummyEnum::One);
  checkEnum(dummy_, DummyEnum::Two);
  checkEnum(dummy_, DummyEnum::Three);

  checkString(dummy_, "One");
  checkString(dummy_, "Two");
  checkString(dummy_, "Three");

  ASSERT_THROW(
    dummy_.getString(static_cast<DummyEnum>(0)),
    std::runtime_error
  );
  ASSERT_THROW(
    dummy_.getString(static_cast<DummyEnum>(4)),
    std::runtime_error
  );
  ASSERT_THROW(
    dummy_.getEnum("Zero"),
    std::runtime_error
  );
  ASSERT_THROW(
    dummy_.getEnum("Four"),
    std::runtime_error
  );
}

TEST_F(TestLBArgsEnumConverter, test_enum_converter_config) {
  vrt::collection::balance::LBArgsEnumConverter<DummyEnum> dummy_(
    "dummy", "DummyEnum", {
      {DummyEnum::One,   "One"},
      {DummyEnum::Two,   "Two"},
      {DummyEnum::Three, "Three"}
    }
  );
  // normally this wouldn't reuse the same enum, but we're just testing
  // config-related stuff here so it's fine
  vrt::collection::balance::LBArgsEnumConverter<DummyEnum> count_(
    "count", "DummyEnum", {
      {DummyEnum::One,   "One"},
      {DummyEnum::Two,   "Two"},
      {DummyEnum::Three, "Three"}
    }
  );

  std::string config_string("dummy=Two");  // deliberately omit 'count='
  auto config = vrt::collection::balance::ReadLBConfig::makeConfigFromParams(
    config_string
  );

  // explicitly specified should return specified value
  EXPECT_EQ(dummy_.getFromConfig(&config, DummyEnum::One), DummyEnum::Two);
  // unspecified should return default value
  EXPECT_EQ(count_.getFromConfig(&config, DummyEnum::One), DummyEnum::One);

  std::string bad_config_string("dummy=Four");
  auto bad_config = vrt::collection::balance::ReadLBConfig::makeConfigFromParams(
    bad_config_string
  );

  ASSERT_THROW(
    dummy_.getFromConfig(&bad_config, DummyEnum::One),
    std::runtime_error
  );
  ASSERT_THROW(
    dummy_.getFromConfig(&config, static_cast<DummyEnum>(0)),
    std::runtime_error
  );
}

}}} // end namespace vt::tests::unit
