/*
//@HEADER
// *****************************************************************************
//
//                       test_reduce_collection_handler.h
//                           DARMA Toolkit v. 1.0.0
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

#include "test_reduce_collection_common.h"
#include "test_parallel_harness.h"

#if !defined INCLUDED_TEST_REDUCE_COLLECTION_HANDLER_H
#define INCLUDED_TEST_REDUCE_COLLECTION_HANDLER_H

#define ENABLE_REDUCE_EXPR_CALLBACK 0

namespace vt { namespace tests { namespace unit { namespace reduce {

void colHanBasic(ColMsg* msg, MyCol* col) {

  auto const& idx = col->getIndex();
  debug_print(
    reduce, node,
    "colHan: received: ptr={}, idx={}, getIndex={}\n",
    print_ptr(col), idx.x(), col->getIndex().x()
  );

  auto reduce_msg = vt::makeSharedMessage<MyReduceMsg>(idx.x());
  auto proxy = col->getProxy();
  debug_print(reduce, node, "msg->num={}\n", reduce_msg->num);

  int const expected = collect_size * (collect_size - 1) / 2;

  vt::theCollection()->reduceMsg<
    MyCol, MyReduceMsg, reducePlus<expected>
  >(proxy, reduce_msg);
}

void colHanVec(ColMsg* msg, MyCol* col) {
  auto const& idx = col->getIndex();
  debug_print(
    reduce, node,
    "colHanPartialProxy: received: ptr={}, idx={}, getIndex={}\n",
    print_ptr(col), idx.x(), col->getIndex().x()
  );

  auto reduce_msg = vt::makeSharedMessage<SysMsgVec>(static_cast<double>(idx.x()));
  auto proxy = col->getProxy();
  debug_print(
    reduce, node,
    "msg->vec.size={}\n", reduce_msg->getConstVal().vec.size()
  );

  vt::theCollection()->reduceMsg<
    MyCol,
    SysMsgVec,
    SysMsgVec::msgHandler<SysMsgVec, vt::collective::PlusOp<VectorPayload>, CheckVec>
  >(proxy, reduce_msg);
}

void colHanVecProxy(ColMsg* msg, MyCol* col) {
  auto const& idx = col->getIndex();
  debug_print(
    reduce, node,
    "colHanVecProxy: received: ptr={}, idx={}, getIndex={}\n",
    print_ptr(col), idx.x(), col->getIndex().x()
  );

  auto reduce_msg = vt::makeSharedMessage<SysMsgVec>(static_cast<double>(idx.x()));
  auto proxy = col->getCollectionProxy();
  debug_print(
    reduce, node,
    "msg->vec.size={}\n", reduce_msg->getConstVal().vec.size()
  );
  proxy.reduce<vt::collective::PlusOp<VectorPayload>, CheckVec>(reduce_msg);
}

void colHanVecProxyCB(ColMsg* msg, MyCol* col) {
  auto const& idx = col->getIndex();
  debug_print(
    reduce, node,
    "colHanVecProxyCB: received: ptr={}, idx={}, getIndex={}\n",
    print_ptr(col), idx.x(), col->getIndex().x()
  );

  auto reduce_msg = vt::makeSharedMessage<SysMsgVec>(static_cast<double>(idx.x()));
  auto proxy = col->getCollectionProxy();
  debug_print(
    reduce, node,
    "msg->vec.size={}\n", reduce_msg->getConstVal().vec.size()
  );

  auto cb = vt::theCB()->makeSend<CheckVec>(0);
  vtAssertExpr(cb.valid());
  proxy.reduce<vt::collective::PlusOp<VectorPayload>>(reduce_msg,cb);
}

void colHanNoneCB(ColMsg* msg, MyCol* col) {
  auto const& idx = col->getIndex();
  debug_print(
    reduce, node,
    "colHanNoneCB: received: ptr={}, idx={}, getIndex={}\n",
    print_ptr(col), idx.x(), col->getIndex().x()
  );

  auto rmsg = vt::makeMessage<MyReduceNoneMsg>();
  auto proxy = col->getCollectionProxy();
  auto cb = vt::theCB()->makeSend<NoneReduce>(0);
  vtAssertExpr(cb.valid());
  proxy.reduce(rmsg.get(),cb);
}

// Using reduceExpr with callback is broken and has fundamental flaws.
// These tests are disabled for now.
#if ENABLE_REDUCE_EXPR_CALLBACK
void colHanPartial(ColMsg* msg, MyCol* col) {
  auto const& idx = col->getIndex();

  debug_print(
    reduce, node,
    "colHanPartial: received: ptr={}, idx={}, getIndex={}\n",
    print_ptr(col), idx.x(), col->getIndex().x()
  );

  auto reduce_msg = makeSharedMessage<MyReduceMsg>(idx.x());
  auto proxy = col->getProxy();
  debug_print(reduce, node, "msg->num={}\n", reduce_msg->num);

  int const expected = index_tresh * (index_tresh - 1) / 2;

  theCollection()->reduceMsgExpr<
    MyCol, MyReduceMsg, reducePlus<expected>
  >(
    proxy,reduce_msg, [](Index1D const idx) -> bool {
      return idx.x() < index_tresh;
    }
  );
}

void colHanPartialMulti(ColMsg* msg, MyCol* col) {
  auto const& idx = col->getIndex();
  auto const& grouping = idx.x() % index_tresh;

  debug_print(
    reduce, node,
    "colHanPartialMulti: received: ptr={}, idx={}, getIndex={}\n",
    print_ptr(col), idx.x(), col->getIndex().x()
  );

  auto reduce_msg = makeSharedMessage<MyReduceMsg>(idx.x());
  auto proxy = col->getProxy();
  debug_print(reduce, node, "msg->num={}\n", reduce_msg->num);

//  template parameters deduction fails within 'reduceMsgExpr'
//  if 'expected' is not resolved at compile-time.
//  So just disable expected value checking for now.

//  int expected = 0;
//  for (int value = grouping; value < collect_size; value += index_tresh) {
//    expected += value;
//  }

  theCollection()->reduceMsgExpr<
    MyCol, MyReduceMsg, reducePlus<collect_size,false>
  >(
    proxy,reduce_msg, [=](Index1D const idx) -> bool {
      return idx.x() % index_tresh == grouping;
    }, no_epoch, grouping
  );
}

void colHanPartialProxy(ColMsg* msg, MyCol* col) {
  auto const& idx = col->getIndex();

  debug_print(
    reduce, node,
    "colHanPartialProxy: received: ptr={}, idx={}, getIndex={}\n",
    print_ptr(col), idx.x(), col->getIndex().x()
  );

  auto reduce_msg = makeSharedMessage<MyReduceMsg>(idx.x());
  auto proxy = col->getCollectionProxy();
  debug_print(reduce, node, "msg->num={}\n", reduce_msg->num);

  proxy.reduceExpr< MyReduceMsg, reducePlus<index_tresh> >(
    reduce_msg, [](Index1D const& idx) -> bool {
      return idx.x() < index_tresh;
    }
  );
}
#endif

}}}} // end namespace vt::tests::unit::reduce
#endif /*INCLUDED_TEST_REDUCE_COLLECTION_HANDLER_H*/
