/*
//@HEADER
// *****************************************************************************
//
//                                tutorial_1h.h
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

#include "vt/transport.h"

namespace vt { namespace tutorial {

/// [Tutorial1H]
//                       Reduce Message VT Base Class
//                 \--------------------------------------------/
//                  \                                          /
//                   \                            Reduce Data /
//                    \                          \-----------/
//                     \                          \         /
struct ReduceDataMsg : ::vt::collective::ReduceTMsg<int32_t> {};


// Functor that is the target of the reduction
struct ReduceResult {
  void operator()(ReduceDataMsg* msg) {
    NodeType const num_nodes = ::vt::theContext()->getNumNodes();
    fmt::print("reduction value={}\n", msg->getConstVal());
    assert(num_nodes * 50 == msg->getConstVal());
    (void)num_nodes;  // don't warn about unused value when not debugging
  }
};


// Tutorial code to demonstrate using a callback
static inline void activeMessageReduce() {
  NodeType const this_node = ::vt::theContext()->getNode();
  (void)this_node;  // don't warn about unused variable
  NodeType const num_nodes = ::vt::theContext()->getNumNodes();
  (void)num_nodes;  // don't warn about unused variable

  /*
   * Perform reduction over all the nodes.
   */

  // This is the type of the reduction (uses the plus operator over the data
  // type). Once can implement their own data type and overload the plus
  // operator for the combine during the reduce
  using ReduceOp = ::vt::collective::PlusOp<int32_t>;

  NodeType const root_reduce_node = 0;

  auto reduce_msg = ::vt::makeMessage<ReduceDataMsg>();

  // Get a reference to the value to set it in this reduce msg
  reduce_msg->getVal() = 50;

  auto const default_proxy = theObjGroup()->getDefault();
  default_proxy.reduceMsg<ReduceOp, ReduceResult>(
    root_reduce_node, reduce_msg.get()
  );
}
/// [Tutorial1H]

}} /* end namespace vt::tutorial */
