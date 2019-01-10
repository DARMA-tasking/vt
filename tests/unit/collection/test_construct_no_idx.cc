/*
//@HEADER
// ************************************************************************
//
//                          test_construct_no_idx.cc
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_collection_common.h"
#include "test_collection_construct_common.h"

#include <cstdint>
#include <tuple>
#include <string>

namespace vt { namespace tests { namespace unit {

namespace multi_param_no_idx_ {
template <typename... Args> struct ColMsg;
template <typename... Args>
struct TestCol : Collection<TestCol<Args...>,TestIndex>, BaseCol {
  using MsgType = ColMsg<Args...>;
  using ParamType = std::tuple<Args...>;
  TestCol() = default;
  TestCol(Args... args)
    : Collection<TestCol, TestIndex>(),
      BaseCol(true)
  {
    #if PRINT_CONSTRUCTOR_VALUES
      ConstructTuple<ParamType>::print(std::make_tuple(args...));
    #endif
    ConstructTuple<ParamType>::isCorrect(std::make_tuple(args...));
  }
};
template <typename... Args>
struct ColMsg : CollectionMessage<TestCol<Args...>> {};
} /* end namespace multi_param_no_idx_ */

using CollectionTestTypes = testing::Types<
  multi_param_no_idx_            ::TestCol<int32_t>,
  multi_param_no_idx_            ::TestCol<int64_t>,
  multi_param_no_idx_            ::TestCol<std::string>,
  multi_param_no_idx_            ::TestCol<test_data::A>,
  multi_param_no_idx_            ::TestCol<test_data::B>,
  multi_param_no_idx_            ::TestCol<test_data::C>,
  multi_param_no_idx_            ::TestCol<int32_t,int32_t>,
  multi_param_no_idx_            ::TestCol<int64_t,int64_t>
>;

INSTANTIATE_TYPED_TEST_CASE_P(
  test_construct_no_idx, TestConstruct, CollectionTestTypes
);

INSTANTIATE_TYPED_TEST_CASE_P(
  test_construct_no_idx_dist, TestConstructDist, CollectionTestTypes
);

}}} // end namespace vt::tests::unit
