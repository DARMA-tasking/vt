/*
//@HEADER
// *****************************************************************************
//
//                       test_reduce_collection_handler.h
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

#if !defined INCLUDED_UNIT_COLLECTION_TEST_REDUCE_COLLECTION_HANDLER_H
#define INCLUDED_UNIT_COLLECTION_TEST_REDUCE_COLLECTION_HANDLER_H

#include "test_reduce_collection_common.h"
#include "test_parallel_harness.h"

namespace vt { namespace tests { namespace unit { namespace reduce {

void colHanBasic(MyCol* col) {
  auto const& idx = col->getIndex();
  vt_debug_print(
    normal, reduce,
    "colHan: received: ptr={}, idx={}, getIndex={}\n",
    print_ptr(col), idx.x(), col->getIndex().x()
  );

  auto proxy = col->getCollectionProxy();

  int val = idx.x();
  proxy.reduce<&MyCol::checkNum, collective::PlusOp>(proxy[0], val);
}

void colHanBasicAR(MyCol* col) {
  auto const& idx = col->getIndex();
  vt_debug_print(
    normal, reduce,
    "colHanPartialProxy: received: ptr={}, idx={}, getIndex={}\n",
    print_ptr(col), idx.x(), col->getIndex().x()
  );

  auto proxy = col->getCollectionProxy();
  int val = idx.x();
  proxy.allreduce<&MyCol::checkNum, collective::PlusOp>(val);
}

void colHanVecProxy(MyCol* col) {
  auto const& idx = col->getIndex();
  vt_debug_print(
    normal, reduce,
    "colHanVecProxy: received: ptr={}, idx={}, getIndex={}\n",
    print_ptr(col), idx.x(), col->getIndex().x()
  );

  auto proxy = col->getCollectionProxy();

  VectorPayload vp{static_cast<double>(idx.x())};
  proxy.reduce<&MyCol::checkVec, vt::collective::PlusOp>(proxy[0], vp);
}

void colHanVecProxyAR(MyCol* col) {
  auto const& idx = col->getIndex();
  vt_debug_print(
    normal, reduce,
    "colHanVecProxyCB: received: ptr={}, idx={}, getIndex={}\n",
    print_ptr(col), idx.x(), col->getIndex().x()
  );

  auto proxy = col->getCollectionProxy();

  VectorPayload vp{static_cast<double>(idx.x())};
  proxy.allreduce<&MyCol::checkVec, vt::collective::PlusOp>(vp);
}

void colHanNoneCB(MyCol* col) {
  auto const& idx = col->getIndex();
  vt_debug_print(
    normal, reduce,
    "colHanNoneCB: received: ptr={}, idx={}, getIndex={}\n",
    print_ptr(col), idx.x(), col->getIndex().x()
  );

  auto proxy = col->getCollectionProxy();
  proxy.reduce<&MyCol::noneReduce>(proxy[0]);
}

}}}} // end namespace vt::tests::unit::reduce
#endif /*INCLUDED_UNIT_COLLECTION_TEST_REDUCE_COLLECTION_HANDLER_H*/
