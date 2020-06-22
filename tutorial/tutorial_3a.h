/*
//@HEADER
// *****************************************************************************
//
//                                tutorial_3a.h
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

/// [Tutorial3A]
//              VT Base Message
//             \----------------/
//              \              /
struct ExampleMsg : ::vt::Message {
  ExampleMsg() = default;
  explicit ExampleMsg(int32_t in_ttl) : ttl(in_ttl-1) { }

  int32_t ttl = 0;
};

// Forward declaration for the active message handler
static void recurHandler(ExampleMsg* msg);

// Tutorial code to demonstrate using a callback
static inline void activeMessageTerm() {
  NodeType const this_node = ::vt::theContext()->getNode();
  NodeType const num_nodes = ::vt::theContext()->getNumNodes();
  (void)num_nodes;  // don't warn about unused variable

  /*
   * Termination will allow us to track a subcomputation with causality to
   * determine when a sub-computation terminated in a distributed-manner. The
   * tutorial demonstrates how to use `collective` epochs. Rooted epoch will be
   * demonstrated in a follow-on tutorial.
   */

  // Create a new epoch: this is a collective invocation
  auto const new_epoch = theTerm()->makeEpochCollective();

  if (this_node == 0) {
    auto msg = vt::makeMessage<ExampleMsg>(8);
    envelopeSetEpoch(msg->env, new_epoch);
    vt::theMsg()->sendMsg<ExampleMsg,recurHandler>(this_node+1,msg.get());
  }

  // Any node that wishes to have a notification on termination for a given
  // epoch can add actions for the termination detector
  theTerm()->addAction(
    new_epoch, []{
      auto const node = vt::theContext()->getNode();
      fmt::print("{}: recurHandler terminated\n", node);
    }
  );

  // This is not explicitly a collective, but all nodes need to call
  // `finishedEpoch` to tell the system they are finished sending messages
  // for the epoch.
  theTerm()->finishedEpoch(new_epoch);
}

// Message handler that recursively sends messages
static void recurHandler(ExampleMsg* msg) {
  NodeType const num_nodes = ::vt::theContext()->getNumNodes();
  NodeType const this_node = ::vt::theContext()->getNode();

  ::fmt::print(
    "{}: recurHandler: ttl={}, triggered\n", this_node, msg->ttl
  );

  if (msg->ttl > 0) {
    auto const num_send = static_cast<int32_t>(drand48() * 3);
    for (auto i = 0; i < num_send; i++) {
      auto next_node = (this_node + 1 > num_nodes - 1) ? 0 : (this_node + 1);

      ::fmt::print(
        "{}: recurHandler: i={}, next_node={}, num_send={}\n",
        this_node, i, next_node, num_send
      );

      auto msg_send = vt::makeMessage<ExampleMsg>(msg->ttl);
      vt::theMsg()->sendMsg<ExampleMsg,recurHandler>(next_node,msg_send.get());
    }
  }
}
/// [Tutorial3A]

}} /* end namespace vt::tutorial */
