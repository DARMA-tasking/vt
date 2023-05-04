/*
//@HEADER
// *****************************************************************************
//
//                                tutorial_1h.h
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

#include "vt/transport.h"

namespace vt { namespace tutorial {

/// [Tutorial1H]
void reduceResult(int result) {
  NodeType const num_nodes = ::vt::theContext()->getNumNodes();
  (void)num_nodes;  // don't warn about possibly unused variable
  fmt::print("reduction value={}\n", result);
  assert(num_nodes * 50 == result);
}

// Tutorial code to demonstrate using reduction on all nodes
static inline void activeMessageReduce() {
  NodeType const this_node = ::vt::theContext()->getNode();
  (void)this_node;  // don't warn about unused variable
  NodeType const num_nodes = ::vt::theContext()->getNumNodes();
  (void)num_nodes;  // don't warn about unused variable

  /*
   * Perform reduction over all the nodes.
   */
  NodeType const root_reduce_node = 0;

  auto r = vt::theCollective()->global();
  r->reduce<reduceResult, collective::PlusOp>(vt::Node{root_reduce_node}, 50);
}
/// [Tutorial1H]

}} /* end namespace vt::tutorial */
