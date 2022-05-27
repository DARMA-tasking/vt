/*
//@HEADER
// *****************************************************************************
//
//                            test_send.extended.cc
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
#include "test_collection_common.h"
#include "data_message.h"
#include "test_send.h"

#include <cstdint>
#include <tuple>

namespace vt { namespace tests { namespace unit { namespace send {

TYPED_TEST_P(TestCollectionSend, test_collection_send_extended_1) {
  test_collection_send_1<TypeParam>();
}

TYPED_TEST_P(TestCollectionSendMem, test_collection_send_ptm_extended_1) {
  test_collection_send_ptm_1<TypeParam>();
}

REGISTER_TYPED_TEST_SUITE_P(TestCollectionSend, test_collection_send_extended_1);
REGISTER_TYPED_TEST_SUITE_P(TestCollectionSendMem, test_collection_send_ptm_extended_1);

using CollectionTestTypesExtended = testing::Types<
  send_col_            ::TestCol<int64_t>,
  send_col_            ::TestCol<std::string>,
  send_col_            ::TestCol<test_data::A>,
  send_col_            ::TestCol<test_data::B>,
  send_col_            ::TestCol<test_data::C>,
  send_col_            ::TestCol<int32_t,int32_t>,
  send_col_            ::TestCol<int64_t,int64_t>
>;

INSTANTIATE_TYPED_TEST_SUITE_P(
  test_collection_send_extended, TestCollectionSend,
  CollectionTestTypesExtended, DEFAULT_NAME_GEN
);
INSTANTIATE_TYPED_TEST_SUITE_P(
  test_collection_send_mem_extended, TestCollectionSendMem,
  CollectionTestTypesExtended, DEFAULT_NAME_GEN
);

}}}} // end namespace vt::tests::unit::send
