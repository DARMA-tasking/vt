/*
//@HEADER
// ************************************************************************
//
//                    test_reduce_collection.cc
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

#include "test_reduce_collection_handler.h"

namespace vt { namespace tests { namespace unit {

struct TestReduceCollection : TestParallelHarnessParam<int> {};

TEST_P(TestReduceCollection, test_reduce_op) {
  using namespace reduce;

  auto const my_node = theContext()->getNode();
  auto const root = 0;

  if (my_node == root) {
    auto reduce_case = GetParam();
    auto size = (reduce_case == 5 ? collect_size * 4 : collect_size);
    auto const& range = Index1D(size);
    auto proxy = theCollection()->construct<MyCol>(range);
    auto msg = makeSharedMessage<ColMsg>(my_node);

    switch (reduce_case) {
      case 0: proxy.broadcast<ColMsg, colHanBasic>(msg); break;
      case 1: proxy.broadcast<ColMsg, colHanVec>(msg); break;
      case 2: proxy.broadcast<ColMsg, colHanVecProxy>(msg); break;
      case 3: proxy.broadcast<ColMsg, colHanVecProxyCB>(msg); break;

      #if ENABLE_REDUCE_EXPR_CALLBACK
        case 4: proxy.broadcast<ColMsg, colHanPartial>(msg); break;
        case 5: proxy.broadcast<ColMsg, colHanPartialMulti>(msg); break;
        case 6: proxy.broadcast<ColMsg, colHanPartialProxy>(msg); break;
      #endif
      default: vtAbort("Failure: should not be reached");
    }
  }
}

INSTANTIATE_TEST_CASE_P(
  #if ENABLE_REDUCE_EXPR_CALLBACK
    InstantiationName, TestReduceCollection, ::testing::Range(0, 7)
  #else
    InstantiationName, TestReduceCollection, ::testing::Range(0, 4)
  #endif
);

}}} // end namespace vt::tests::unit