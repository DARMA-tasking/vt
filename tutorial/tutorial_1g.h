/*
//@HEADER
// *****************************************************************************
//
//                                tutorial_1g.h
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

/// [Tutorial1G]
//              VT Base Message
//             \----------------/
//              \              /
struct DataMsg : ::vt::Message { };

struct MsgWithCallback : ::vt::Message {
  MsgWithCallback() = default;
  explicit MsgWithCallback(Callback<DataMsg> in_cb) : cb(in_cb) {}

  Callback<DataMsg> cb;
};


// Forward declaration for the active message handler
static void getCallbackHandler(MsgWithCallback* msg);

// An active message handler used as the target for a callback
static void callbackHandler(DataMsg* msg) {
  NodeType const cur_node = ::vt::theContext()->getNode();
  ::fmt::print("{}: triggering active message callback\n", cur_node);
}

// An active message handler used as the target for a callback
static void callbackBcastHandler(DataMsg* msg) {
  NodeType const cur_node = ::vt::theContext()->getNode();
  ::fmt::print("{}: triggering active message callback bcast\n", cur_node);
}

// A simple context object
struct MyContext { };
static MyContext ctx = {};

// A message handler with context used as the target for a callback
static void callbackCtx(DataMsg* msg, MyContext* cbctx) {
  NodeType const cur_node = ::vt::theContext()->getNode();
  ::fmt::print("{}: triggering context callback\n", cur_node);
}


// Tutorial code to demonstrate using a callback
static inline void activeMessageCallback() {
  NodeType const this_node = ::vt::theContext()->getNode();
  NodeType const num_nodes = ::vt::theContext()->getNumNodes();
  (void)num_nodes;  // don't warn about unused variable

  /*
   * Callbacks allow one to generalize the notion of an endpoint with a abstract
   * interface the callee can use without changing code. A callback can trigger
   * a lambda, handler on a node, handler broadcast, handler/lambda with a
   * context, message send of virtual context collection (element or broadcast)
   */

  if (this_node == 0) {
    // Node sending the callback message to, which shall invoke the callback
    NodeType const to_node = 1;
    // Node that we want to callback to execute on
    NodeType const cb_node = 0;

    // Example lambda callback (void)
    auto void_fn = [=]{
      ::fmt::print("{}: triggering void function callback\n", this_node);
    };

    // Example of a void lambda callback
    {
      auto cb = ::vt::theCB()->makeFunc(vt::pipe::LifetimeEnum::Once, void_fn);
      auto msg = ::vt::makeMessage<MsgWithCallback>(cb);
      ::vt::theMsg()->sendMsg<MsgWithCallback,getCallbackHandler>(to_node, msg);
    }

    // Example of active message handler callback with send node
    {
      auto cb = ::vt::theCB()->makeSend<DataMsg,callbackHandler>(cb_node);
      auto msg = ::vt::makeMessage<MsgWithCallback>(cb);
      ::vt::theMsg()->sendMsg<MsgWithCallback,getCallbackHandler>(to_node, msg);
    }

    // Example of active message handler callback with broadcast
    {
      auto cb = ::vt::theCB()->makeBcast<DataMsg,callbackBcastHandler>();
      auto msg = ::vt::makeMessage<MsgWithCallback>(cb);
      ::vt::theMsg()->sendMsg<MsgWithCallback,getCallbackHandler>(to_node, msg);
    }

    // Example of context callback
    {
      auto cb = ::vt::theCB()->makeFunc<DataMsg,MyContext>(
        vt::pipe::LifetimeEnum::Once, &ctx, callbackCtx
      );
      auto msg = ::vt::makeMessage<MsgWithCallback>(cb);
      ::vt::theMsg()->sendMsg<MsgWithCallback,getCallbackHandler>(to_node, msg);
    }
  }
}

// Message handler for to receive callback and invoke it
static void getCallbackHandler(MsgWithCallback* msg) {
  auto const cur_node = ::vt::theContext()->getNode();
  ::fmt::print("getCallbackHandler: triggered on node={}\n", cur_node);

  // Send the callback a message
  msg->cb.send<DataMsg>();
}
/// [Tutorial1G]

}} /* end namespace vt::tutorial */
