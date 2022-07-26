/*
//@HEADER
// *****************************************************************************
//
//                              test_broadcast.cc
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

#include "test_collection_common.h"
#include "data_message.h"
#include "test_broadcast.h"

#include <cstdint>

namespace vt { namespace tests { namespace unit { namespace bcast {

TYPED_TEST_P(TestBroadcast, test_broadcast_basic_1) {
  test_broadcast_1<TypeParam>("test_broadcast_basic_1");
}

TYPED_TEST_P(TestBroadcastDynamic, test_broadcast_dynamic_basic_1) {
  test_broadcast_dynamic_1<TypeParam>("test_broadcast_dynamic_basic_1");
}

std::unordered_map<TestIndex, bool> DynamicCountFun::index_map{};

REGISTER_TYPED_TEST_SUITE_P(TestBroadcast, test_broadcast_basic_1);
REGISTER_TYPED_TEST_SUITE_P(TestBroadcastDynamic, test_broadcast_dynamic_basic_1);

using CollectionTestTypesBasic = testing::Types<
  bcast_col_            ::TestCol<int32_t>
>;

INSTANTIATE_TYPED_TEST_SUITE_P(
  test_bcast_basic, TestBroadcast, CollectionTestTypesBasic, DEFAULT_NAME_GEN
);

INSTANTIATE_TYPED_TEST_SUITE_P(
  test_bcast_basic, TestBroadcastDynamic, CollectionTestTypesBasic, DEFAULT_NAME_GEN
);

}}}} // end namespace vt::tests::unit::bcast
