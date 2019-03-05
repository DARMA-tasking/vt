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

using namespace vt;
using namespace vt::collective;
using namespace vt::tests::unit;
using namespace vt::tests::unit::reduce;

struct TestReduceCollection : TestParallelHarness {};

TEST_F(TestReduceCollection, test_reduce_op) {
  auto const my_node = theContext()->getNode();
  auto const root = 0;

  if (my_node == root) {
    auto const& range = Index1D(collect_size);
    auto proxy = theCollection()->construct<MyCol>(range);
    auto msg = new ColMsg(my_node);
    proxy.broadcast<ColMsg,colHanBasic>(msg);
  }
}

TEST_F(TestReduceCollection, test_reduce_vec_op) {
  auto const my_node = theContext()->getNode();
  auto const root = 0;

  if (my_node == root) {
    auto const& range = Index1D(collect_size);
    auto proxy = theCollection()->construct<MyCol>(range);
    auto msg = new ColMsg(my_node);
    proxy.broadcast<ColMsg,colHanVec>(msg);
  }
}

TEST_F(TestReduceCollection, test_reduce_vec_proxy_op) {
  auto const my_node = theContext()->getNode();
  auto const root = 0;

  if (my_node == root) {
    auto const& range = Index1D(collect_size);
    auto proxy = theCollection()->construct<MyCol>(range);
    auto msg = new ColMsg(my_node);
    proxy.broadcast<ColMsg,colHanVecProxy>(msg);
  }
}

TEST_F(TestReduceCollection, test_reduce_vec_proxy_callback_op) {
  auto const my_node = theContext()->getNode();
  auto const root = 0;

  if (my_node == root) {
    auto const& range = Index1D(collect_size);
    auto proxy = theCollection()->construct<MyCol>(range);
    auto msg = new ColMsg(my_node);
    proxy.broadcast<ColMsg,colHanVecProxyCB>(msg);
  }
}

#if ENABLE_REDUCE_EXPR
TEST_F(TestReduceCollection, test_reduce_partial_op) {
  auto const my_node = theContext()->getNode();
  auto const root = 0;

  if (my_node == root) {
    auto const& range = Index1D(collect_size);
    auto proxy = theCollection()->construct<MyCol>(range);
    auto msg = new ColMsg(my_node);
    proxy.broadcast<ColMsg,colHanPartial>(msg);
  }
}

TEST_F(TestReduceCollection, test_reduce_partial_proxy_op) {
  auto const my_node = theContext()->getNode();
  auto const root = 0;

  if (my_node == root) {
    auto const& range = Index1D(collect_size);
    auto proxy = theCollection()->construct<MyCol>(range);
    auto msg = new ColMsg(my_node);
    proxy.broadcast<ColMsg,colHanPartialProxy>(msg);
  }
}

TEST_F(TestReduceCollection, test_reduce_partial_multi_op) {
  auto const my_node = theContext()->getNode();
  auto const root = 0;

  if (my_node == root) {
    auto const& range = Index1D(collect_size * 4);
    auto proxy = theCollection()->construct<MyCol>(range);
    auto msg = new ColMsg(my_node);
    proxy.broadcast<ColMsg,colHanPartialMulti>(msg);
  }
}
#endif

}}} // end namespace vt::tests::unit