/*
//@HEADER
// *****************************************************************************
//
//                                tutorial_1e.h
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

/// [Tutorial1E]
//                  VT Base Message
//                 \----------------/
//                  \              /
struct MySimpleMsg : ::vt::Message { };

// Forward declaration for the active message handler
static void msgHandlerGroupA(MySimpleMsg* msg);

// Tutorial code to demonstrate broadcasting a message to the entire system
static inline void activeMessageGroupRoot() {
  NodeType const this_node = ::vt::theContext()->getNode();
  NodeType const num_nodes = ::vt::theContext()->getNumNodes();

  /*
   * This is an example of the rooted group creation and broadcast to that
   * group. A group allows the user to create a subset of nodes. A broadcast by
   * default sends the message to every node in the default group (which
   * includes all nodes). If a explicit group is set in the envelope, the
   * broadcast will only arrive on the nodes in that group.
   */

  if (this_node == 0) {
    // Create range for the group [1,num_nodes);
    auto range = std::make_unique<::vt::group::region::Range>(1, num_nodes);
    // The non-collective group is created by a single node based on a range or
    // list of nodes. The lambda is executed once the group is created. By
    // setting the group in the envelope of the message and broadcasting the
    // message will arrive on the set of nodes included in the group
    auto id = theGroup()->newGroup(std::move(range), [](GroupType group_id){
      fmt::print("Group is created: id={:x}\n", group_id);
      auto msg = makeMessage<MySimpleMsg>();
      envelopeSetGroup(msg->env, group_id);
      theMsg()->broadcastMsg<MySimpleMsg,msgHandlerGroupA>(msg);
    });
    // The `id' that is returned from the newGroup invocation, can be used
    // anywhere in the system to broadcast (multicast) to this group.
    (void)id;  // don't warn about unused variable
  }
}

// Message handler
static void msgHandlerGroupA(MySimpleMsg* msg) {
  auto const cur_node = ::vt::theContext()->getNode();
  vtAssert(cur_node >= 1, "This handler should only execute on nodes >= 1");

  ::fmt::print("msgHandlerGroupA: triggered on node={}\n", cur_node);
}
/// [Tutorial1E]

}} /* end namespace vt::tutorial */
