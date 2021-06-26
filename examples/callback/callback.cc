/*
//@HEADER
// *****************************************************************************
//
//                                 callback.cc
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

#include <vt/transport.h>

/// [Callback examples]
// Message sent from the callback to the callback endpoint
struct TestMsg : vt::Message {
  using MessageParentType = ::vt::Message;
  vt_msg_serialize_required(); // for string

  TestMsg() = default;

  explicit TestMsg(int in_val, std::string const& in_s = "hello")
    : val_(in_val),
      s_(in_s)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);
    s | val_;
    s | s_;
  }

  int val_ = 0;
  std::string s_;
};

// Message containing the callback to invoke
struct HelloMsg : vt::Message {
  explicit HelloMsg(vt::Callback<TestMsg> in_cb)
    : cb_(in_cb)
  { }

  vt::Callback<TestMsg> cb_;
};

// Handler function to invoke the callback from
void hello_world(HelloMsg* msg) {
  static int val = 1;
  fmt::print("{}: Sending callback\n", vt::theContext()->getNode());
  msg->cb_.send(292 + val++, "test string");
}

void printOutput(TestMsg* msg, std::string type) {
  vt::NodeType this_node = vt::theContext()->getNode();
  fmt::print("{}: cb {}: val={}, str={}\n", this_node, type, msg->val_, msg->s_);
}

// Functor callback endpoint
struct CallbackFunctor {
  void operator()(TestMsg* msg) {
    printOutput(msg, "CallbackFunctor");
  }
};

// Function callback endpoint
static void callbackFunc(TestMsg* msg) {
  printOutput(msg, "callbackFunc");
}

struct MyObj {
  // Objgroup callback endpoint
  void handler(TestMsg* msg) {
    printOutput(msg, "MyObj::handler");
  }
};

struct MyCol : vt::Collection<MyCol, vt::Index1D> { };

// Collection handler callback endpoint
void colHan(TestMsg* msg, MyCol* col) {
  printOutput(msg, "MyCol colHan (non-intrusive)");
}

void bounceCallback(vt::Callback<TestMsg> cb) {
  auto msg = vt::makeMessage<HelloMsg>(cb);
  vt::theMsg()->sendMsg<HelloMsg, hello_world>(1, msg);
}

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  vt::NodeType this_node = vt::theContext()->getNode();
  vt::NodeType num_nodes = vt::theContext()->getNumNodes();

  if (num_nodes == 1) {
    return vt::rerror("requires at least 2 nodes");
  }

  auto obj = vt::theObjGroup()->makeCollective<MyObj>();
  auto col = vt::theCollection()->constructCollective<MyCol>(vt::Index1D(8));

  if (this_node == 0) {
    vt::NodeType dest = num_nodes > 2 ? 2 : 0;

    auto cb_functor = vt::theCB()->makeSend<CallbackFunctor>(dest);
    bounceCallback(cb_functor);

    auto cb_func = vt::theCB()->makeSend<TestMsg,callbackFunc>(dest);
    bounceCallback(cb_func);

    auto cb_obj = vt::theCB()->makeSend<MyObj,TestMsg,&MyObj::handler>(obj[dest]);
    bounceCallback(cb_obj);

    auto cb_obj_bcast = vt::theCB()->makeBcast<MyObj,TestMsg,&MyObj::handler>(obj);
    bounceCallback(cb_obj_bcast);

    auto cb_col = vt::theCB()->makeSend<MyCol,TestMsg,colHan>(col[5]);
    bounceCallback(cb_col);

    auto cb_col_bcast = vt::theCB()->makeBcast<MyCol,TestMsg,colHan>(col);
    bounceCallback(cb_col_bcast);
  }

  vt::finalize();

  return 0;
}
/// [Callback examples]
