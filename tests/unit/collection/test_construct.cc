/*
//@HEADER
// ************************************************************************
//
//                          test_construct.cc
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

#include "test_collection_common.h"
#include "test_collection_construct_common.h"

#include <cstdint>
#include <tuple>
#include <string>

namespace vt { namespace tests { namespace unit {

namespace default_ {
struct ColMsg;
struct TestCol : Collection<TestCol,TestIndex> {
  using MsgType = ColMsg;
  using ParamType = std::tuple<>;
  TestCol()
    : Collection<TestCol, TestIndex>(),
      constructed_(true)
  { }
  bool isConstructed() const { return constructed_; }
private:
  bool constructed_ = false;
};
struct ColMsg : CollectionMessage<TestCol> { };
} /* end namespace default_ */

namespace index_ {
struct ColMsg;
struct TestCol : Collection<TestCol,TestIndex> {
  using MsgType = ColMsg;
  using ParamType = std::tuple<>;
  TestCol() = default;
  TestCol(TestIndex idx)
    : Collection<TestCol, TestIndex>(),
      constructed_(true)
  { }
  bool isConstructed() const { return constructed_; }
private:
  bool constructed_ = false;
};
struct ColMsg : CollectionMessage<TestCol> { };
} /* end namespace default_ */

/*
 *  These are now split into multiple files..this greatly improves compile-time
 *  for the tests
 */
// using CollectionTestTypes = testing::Types<
//   default_                       ::TestCol,
//   index_                         ::TestCol,
//   multi_param_idx_fst_           ::TestCol<int32_t>,
//   multi_param_idx_fst_           ::TestCol<int64_t>,
//   multi_param_idx_fst_           ::TestCol<std::string>,
//   multi_param_idx_fst_           ::TestCol<test_data::A>,
//   multi_param_idx_fst_           ::TestCol<test_data::B>,
//   multi_param_idx_fst_           ::TestCol<test_data::C>,
//   multi_param_idx_fst_           ::TestCol<int32_t,int32_t>,
//   multi_param_idx_fst_           ::TestCol<int64_t,int64_t>,
//   multi_param_idx_snd_           ::TestCol<int32_t>,
//   multi_param_idx_snd_           ::TestCol<int64_t>,
//   multi_param_idx_snd_           ::TestCol<std::string>,
//   multi_param_idx_snd_           ::TestCol<test_data::A>,
//   multi_param_idx_snd_           ::TestCol<test_data::B>,
//   multi_param_idx_snd_           ::TestCol<test_data::C>,
//   multi_param_idx_snd_           ::TestCol<int32_t,int32_t>,
//   multi_param_idx_snd_           ::TestCol<int64_t,int64_t>,
//   multi_param_no_idx_            ::TestCol<int32_t>,
//   multi_param_no_idx_            ::TestCol<int64_t>,
//   multi_param_no_idx_            ::TestCol<std::string>,
//   multi_param_no_idx_            ::TestCol<test_data::A>,
//   multi_param_no_idx_            ::TestCol<test_data::B>,
//   multi_param_no_idx_            ::TestCol<test_data::C>,
//   multi_param_no_idx_            ::TestCol<int32_t,int32_t>,
//   multi_param_no_idx_            ::TestCol<int64_t,int64_t>
// >;

using CollectionTestTypes = testing::Types<
  default_                       ::TestCol,
  index_                         ::TestCol
>;

using CollectionTestDistTypes = testing::Types<
  default_                       ::TestCol
>;

INSTANTIATE_TYPED_TEST_CASE_P(
  test_construct_simple, TestConstruct, CollectionTestTypes
);

INSTANTIATE_TYPED_TEST_CASE_P(
  test_construct_distributed_simple, TestConstructDist, CollectionTestDistTypes
);

}}} // end namespace vt::tests::unit
