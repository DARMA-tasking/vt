/*
//@HEADER
// *****************************************************************************
//
//                                 zoltanlb.cc
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_ZOLTANLB_ZOLTANLB_CC
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_ZOLTANLB_ZOLTANLB_CC

#include "vt/config.h"
#include "vt/vrt/collection/balance/zoltanlb/zoltanlb.h"

#include <zoltan.h>

namespace vt { namespace vrt { namespace collection { namespace lb {

void ZoltanLB::init(objgroup::proxy::Proxy<ZoltanLB> in_proxy) {
  proxy = in_proxy;
}

void ZoltanLB::inputParams(balance::SpecEntry* spec) { }

void ZoltanLB::runLB() {
  initZoltan();

  auto const& this_node = theContext()->getNode();
  // auto const& num_nodes = theContext()->getNumNodes();
  // auto const next_node = this_node + 1 > num_nodes-1 ? 0 : this_node + 1;

  if (this_node == 0) {
    vt_print(lb, "ZoltanLB: runLB\n");
    fflush(stdout);
  }

  startMigrationCollective();
  finishMigrationCollective();
}

Zoltan_Struct* ZoltanLB::initZoltan() {
  float ver = 0.0f;
  auto const ret = Zoltan_Initialize(0, nullptr, &ver);
  assert(ret == ZOLTAN_OK);

  auto zoltan = Zoltan_Create(theContext()->getComm());
  return zoltan;
}

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_ZOLTANLB_ZOLTANLB_CC*/
