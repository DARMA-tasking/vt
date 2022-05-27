/*
//@HEADER
// *****************************************************************************
//
//                               test_handler.cc
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

#include "vt/handler/handler.h"

namespace vt { namespace tests { namespace unit {

struct TestHandler : TestParallelHarness { };

TEST_F(TestHandler, test_make_handler_default_params) {
  constexpr bool is_auto = true;
  constexpr bool is_functor = false;
  constexpr HandlerIdentifierType id = 134;
  constexpr auto reg_type = auto_registry::RegistryTypeEnum::RegObjGroup;

  auto const han = HandlerManager::makeHandler(is_auto, is_functor, id, reg_type);

  EXPECT_EQ(is_auto, HandlerManager::isHandlerAuto(han));
  EXPECT_EQ(is_functor, HandlerManager::isHandlerFunctor(han));
  EXPECT_EQ(id, HandlerManager::getHandlerIdentifier(han));
  EXPECT_EQ(reg_type, HandlerManager::getHandlerRegistryType(han));

  // For RegObjGroup is_objgroup is expected to be true
  // and is_member to be false
  constexpr bool is_objgroup = true;
  constexpr bool is_member = false;
  EXPECT_EQ(is_objgroup, HandlerManager::isHandlerObjGroup(han));
  EXPECT_EQ(is_member, HandlerManager::isHandlerMember(han));

  // Default parameters' values
  constexpr HandlerControlType control = 0;

  EXPECT_EQ(control, HandlerManager::getHandlerControl(han));

#if vt_check_enabled(trace_enabled)
  constexpr bool is_trace = true;
  EXPECT_EQ(is_trace, HandlerManager::isHandlerTrace(han));
#endif
}

TEST_F(TestHandler, TestHandler_test_make_handler_custom_params) {
  constexpr bool is_auto = false;
  constexpr bool is_functor = true;
  constexpr HandlerIdentifierType id = 9746;
  constexpr auto reg_type = auto_registry::RegistryTypeEnum::RegVrtCollectionMember;
  constexpr HandlerControlType control = 2289;
  constexpr bool is_trace = false;

  auto const han = HandlerManager::makeHandler(
    is_auto, is_functor, id, reg_type, control, is_trace
  );

  EXPECT_EQ(is_auto, HandlerManager::isHandlerAuto(han));
  EXPECT_EQ(is_functor, HandlerManager::isHandlerFunctor(han));
  EXPECT_EQ(id, HandlerManager::getHandlerIdentifier(han));
  EXPECT_EQ(reg_type, HandlerManager::getHandlerRegistryType(han));
  EXPECT_EQ(control, HandlerManager::getHandlerControl(han));

  // For RegVrtCollectionMember is_objgroup is expected to be false
  // and is_member to be true
  constexpr bool is_objgroup = false;
  constexpr bool is_member = true;
  EXPECT_EQ(is_objgroup, HandlerManager::isHandlerObjGroup(han));
  EXPECT_EQ(is_member, HandlerManager::isHandlerMember(han));

#if vt_check_enabled(trace_enabled)
  EXPECT_EQ(is_trace, HandlerManager::isHandlerTrace(han));
#endif
}

}}} // end namespace vt::tests::unit
