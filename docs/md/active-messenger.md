
\page active-messenger Active Messenger
\brief Asynchronous send/receive of messages

The active messenger `vt::messaging::ActiveMessenger`, accessed via
`vt::theMsg()`, asynchronously sends and receives messages across nodes using
MPI internally. When sending a message, it uses the \vt registry to consistently
dispatch messages and data to handlers (function pointers, functors, or methods)
across nodes.

Each message contains an envelope `vt::Envelope` to store meta-data associated
with the message, such as the destination and handler to trigger when it
arrives. Sending a message entails setting up the envelope, optionally
serializing the message (depending on whether the serialize overload is
present), and then using `MPI_Isend` to asynchronously transfer the bytes to the
destination node. On the receive side, the active messenger is always probing
for an incoming message and begins a transfer when it discovers one. The \vt
\ref scheduler polls the active messenger to make progress on any incoming
messages.

\section am-simple-example Sending a message

\code{.cpp}
#include <vt/transport.h>

#include <vector>

// Declare a serializable message
struct MyMsg : vt::Message {
  using MessageParentType = vt::Message;
  vt_msg_serialize_required(); // for vector

  MyMsg() = default; // default constructor for de-serialize
  MyMsg(int in_val, std::vector<double> const& in_vec)
    : val(in_val),
      my_vec(in_vec)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);
    s | val;
    s | my_vec;
  }

  int val = 0;
  std::vector<double> my_vec;
};

// Active function pointer
void myHandler(MyMsg* m) {
  vt::NodeType this_node = vt::theContext()->getNode();
  fmt::print("{}: val={}, vec size={}\n", this_node, m->val, m->my_vec.size());
}

// Active functor
struct MyFunctor {
  void operator()(MyMsg* m) {
    vt::NodeType this_node = vt::theContext()->getNode();
    fmt::print("{}: val={}, vec size={}\n", this_node, m->val, m->my_vec.size());
  }
};

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  vt::NodeType this_node = vt::theContext()->getNode();

  if (this_node == 0) {
    // spins in scheduler until termination of the enclosed work
    vt::runInEpochRooted([=]{
      std::vector<double> vec_to_send;
      vec_to_send.push_back(29.);
      vec_to_send.push_back(54.);

      auto msg = vt::makeMessage<MyMsg>(10, vec_to_send);
      vt::theMsg()->sendMsg<MyMsg, myHandler>(1, msg.get()); // send to node 1

      auto msg2 = vt::makeMessage<MyMsg>(11, vec_to_send);
      vt::theMsg()->sendMsg<MyFunctor>(1, msg2.get());  // send to node 1
    });
  }

  vt::finalize();
  return 0;
}

\endcode

Program output:

\code{.shell-session}
1: val=10, vec size=2
1: val=11, vec size=2
\endcode
