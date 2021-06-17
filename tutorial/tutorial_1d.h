/*
//@HEADER
// *****************************************************************************
//
//                                tutorial_1d.h
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

/// [Tutorial1D]
//                  VT Base Message
//                 \----------------/
//                  \              /
struct MyDataMsg : ::vt::Message {
  MyDataMsg() = default;
  MyDataMsg(double in_x, double in_y, double in_z)
    : x(in_x), y(in_y), z(in_z)
  { }

  double getX() const { return x; }
  double getY() const { return y; }
  double getZ() const { return z; }

private:
  double x = 0.0, y = 0.0, z = 0.0;
};

// Forward declaration for the active message handler
static void msgHandlerX(MyDataMsg* msg);

// Tutorial code to demonstrate broadcasting a message to the entire system
static inline void activeMessageBroadcast() {
  NodeType const this_node = ::vt::theContext()->getNode();
  NodeType const num_nodes = ::vt::theContext()->getNumNodes();
  (void)num_nodes;  // don't warn about unused variable

  /*
   * default_proxy.broadcast(..) will send the message to every node in the
   * system. Every node will include all the nodes that VT has depending on the
   * MPI communicator passed in or the size attained (number of ranks) when
   * executing MPI init directly in non-interoperability mode.
   *
   * -- Message Ownership --
   * default_proxy.broadcast(..) will create message out of args passed
   * into it. There's an alternative - default_proxy.broadcastMsg(..) - which
   * relinquishes ownership of the message passed to:
   *
   *   auto msg = ::vt::makeMessage<MyDataMsg>(1.0, 2.0, 3.0);
   *   auto const default_proxy = theObjGroup()->getDefault();
   *   default_proxy.broadcastMsg<MyDataMsg, msgHandlerX>(msg);
   *
   *  Most calls to VT that supply a message are expected to relinquish ownership.
   */

  if (this_node == 0) {
    auto const default_proxy = theObjGroup()->getDefault();
    default_proxy.broadcast<MyDataMsg, msgHandlerX>(1.0, 2.0, 3.0);
  }
}

// Message handler
static void msgHandlerX(MyDataMsg* msg) {
  vtAssert(
    msg->getX() == 1.0 && msg->getY() == 2.0 && msg->getZ() == 3.0,
    "Values x,y,z incorrect"
  );

  auto const cur_node = ::vt::theContext()->getNode();
  ::fmt::print("msgHandlerX: triggered on node={}\n", cur_node);
}
/// [Tutorial1D]

}} /* end namespace vt::tutorial */
