/*
//@HEADER
// *****************************************************************************
//
//                                tutorial_1b.h
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

/*
 * This is a user-defined message that can be sent to a node via VT's active
 * message interface. A message in VT must be derived from the type
 * vt::Message. The derived class will include the ``envelope'', which includes
 * the handler, and other information for processing the message
 */

//            VT Base Message
//           \----------------/
//            \              /
struct MyMsg : ::vt::Message {
  // In general, a default constructor is required for the a message because it
  // may be reconstructed by VT
  MyMsg() = default;

  // A normal constructor
  MyMsg(int in_a, int in_b) : a_(in_a), b_(in_b) { }

  int getA() const { return a_; }
  int getB() const { return b_; }

private:
  int a_ = 0, b_ = 0;
};

/*
 * Following are two active message handler declarations. These are the
 * functions that can be remotely invoked by sending a message in VT. VT allows
 * for multiple styles for handlers. The first (msgHandlerA) is the C-style
 * active message handler. It is exactly equivalent (in terms of practical use
 * ramifications) to the functor style, which follows (MsgHandlerB). Either
 * style can be used to define a message handler. The only slight benefit the
 * functor has is uniqueness in type, when enables introspection of the message
 * type. This is realized in the sending side code: the function style requires
 * the message type before the value function template parameter. However, both
 * are *fully type checked* and will not allow the wrong message type to be
 * sent.
 */

// Forward declaration for the active message handler (function ptr style)
static void msgHandlerA(MyMsg* msg);

// Forward declaration for the active message handler (functor style)
struct MsgHandlerB {
  void operator()(MyMsg* msg);
};

static inline void activeMessageNode() {
  NodeType const this_node = ::vt::theContext()->getNode();
  NodeType const num_nodes = ::vt::theContext()->getNumNodes();
  (void)num_nodes;  // don't warn about unused variable

  /*
   * A basic active message send essentially does an
   * ``MPI_Send(..,destination,...)'' to the destination node passed to
   * ::vt::theMsg()->sendMsg(destination). The handler, which is passed as the
   * second template argument, is the function that is triggered when the
   * message arrives on the destination node.
   *
   * The theMsg()->sendMsg(..) does not serialize the message sent to the
   * destination node. Even if the message has a serialize method (the above
   * example does not), it is sent as bytes. This is because sendMsg always just
   * sends the data directly that is passed to it.
   *
   * This example uses the function pointer style compared to the send inside of
   * msgHandlerA, which uses the functor style send.
   */

  if (this_node == 0) {
    NodeType const to_node = 1;
    auto msg = ::vt::makeMessage<MyMsg>(29,32);
    ::vt::theMsg()->sendMsg<MyMsg,msgHandlerA>(to_node, msg.get());
  }
}

static void msgHandlerA(MyMsg* msg) {
  /*
   * VT provides vtAssert (and a half-dozen variants) that replace the simple
   * the assert call found in cassert.
   */
  vtAssert(msg->getA() == 29, "Value a incorrect");
  vtAssert(msg->getB() == 32, "Value b incorrect");

  auto const cur_node = ::vt::theContext()->getNode();

  ::fmt::print("msgHandlerA: triggered on node={}\n", cur_node);

  /* Node 0 sends MyMsg to node 1 in the above code so this should execute on
   * node 1 */
  vtAssert(cur_node == 1, "This handler should execute on node 1");

  /*
   * In response to this message, node 1 sends a message back to node 0. This
   * invocation uses the functor style send.
   */
  NodeType const to_node = 0;
  auto msg2 = ::vt::makeMessage<MyMsg>(10,20);
  ::vt::theMsg()->sendMsg<MsgHandlerB>(to_node, msg2.get());
}

void MsgHandlerB::operator()(MyMsg* msg) {
  auto const cur_node = ::vt::theContext()->getNode();
  vtAssert(msg->getA() == 10, "Value a incorrect");
  vtAssert(msg->getB() == 20, "Value b incorrect");
  vtAssert(cur_node == 0, "This handler should execute on node 0");
  ::fmt::print("msgHandlerB: triggered on node={}\n", cur_node);
}

}} /* end namespace vt::tutorial */
