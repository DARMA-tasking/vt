/*
//@HEADER
// *****************************************************************************
//
//                          test_reduce_collection.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
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

    switch (reduce_case) {
    case 0: proxy.broadcast<ColMsg, colHanBasic>(my_node); break;
    case 1: proxy.broadcast<ColMsg, colHanVec>(my_node); break;
    case 2: proxy.broadcast<ColMsg, colHanVecProxy>(my_node); break;
    case 3: proxy.broadcast<ColMsg, colHanVecProxyCB>(my_node); break;
    case 4: proxy.broadcast<ColMsg, colHanNoneCB>(my_node); break;

      #if ENABLE_REDUCE_EXPR_CALLBACK
    case 5: proxy.broadcast<ColMsg, colHanPartial>(my_node); break;
    case 6: proxy.broadcast<ColMsg, colHanPartialMulti>(my_node); break;
    case 7: proxy.broadcast<ColMsg, colHanPartialProxy>(my_node); break;
      #endif
      default: vtAbort("Failure: should not be reached");
    }
  }
}

#if ENABLE_REDUCE_EXPR_CALLBACK
  INSTANTIATE_TEST_SUITE_P(
    InstantiationName, TestReduceCollection, ::testing::Range(0, 8)
  );
#else
  INSTANTIATE_TEST_SUITE_P(
    InstantiationName, TestReduceCollection, ::testing::Range(0, 5)
  );
#endif

}}} // end namespace vt::tests::unit
