
#include "vt/transport.h"

namespace vt { namespace tutorial {

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
static void callbackCtx(DataMsg* msg, MyContext* ctx) {
  NodeType const cur_node = ::vt::theContext()->getNode();
  ::fmt::print("{}: triggering context callback\n", cur_node);
}


// Tutorial code to demonstrate using a callback
static inline void activeMessageCallback() {
  NodeType const this_node = ::vt::theContext()->getNode();
  NodeType const num_nodes = ::vt::theContext()->getNumNodes();

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
      auto cb = ::vt::theCB()->makeFunc(void_fn);
      auto msg = ::vt::makeSharedMessage<MsgWithCallback>(cb);
      ::vt::theMsg()->sendMsg<MsgWithCallback,getCallbackHandler>(to_node,msg);
    }

    // Example of active message handler callback with send node
    {
      auto cb = ::vt::theCB()->makeSend<DataMsg,callbackHandler>(cb_node);
      auto msg = ::vt::makeSharedMessage<MsgWithCallback>(cb);
      ::vt::theMsg()->sendMsg<MsgWithCallback,getCallbackHandler>(to_node,msg);
    }

    // Example of active message handler callback with broadcast
    {
      auto cb = ::vt::theCB()->makeBcast<DataMsg,callbackBcastHandler>();
      auto msg = ::vt::makeSharedMessage<MsgWithCallback>(cb);
      ::vt::theMsg()->sendMsg<MsgWithCallback,getCallbackHandler>(to_node,msg);
    }

    // Example of context callback
    {
      auto cb = ::vt::theCB()->makeFunc<DataMsg,MyContext>(&ctx, callbackCtx);
      auto msg = ::vt::makeSharedMessage<MsgWithCallback>(cb);
      ::vt::theMsg()->sendMsg<MsgWithCallback,getCallbackHandler>(to_node,msg);
    }
  }
}

// Message handler for to receive callback and invoke it
static void getCallbackHandler(MsgWithCallback* msg) {
  auto const cur_node = ::vt::theContext()->getNode();
  ::fmt::print("getCallbackHandler: triggered on node={}\n", cur_node);

  // Create a msg to trigger to callback
  auto data_msg = ::vt::makeSharedMessage<DataMsg>();
  // Send the callback with the message
  msg->cb.send(data_msg);
}

}} /* end namespace vt::tutorial */
