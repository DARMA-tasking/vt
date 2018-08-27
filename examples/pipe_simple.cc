
#include "transport.h"
#include <cstdlib>

using namespace vt;
using namespace vt::index;
using namespace vt::mapping;

namespace serdes {

template <typename Serializer>
void serialize(Serializer& s, std::string& str) {
  typename std::string::size_type str_size = str.length();
  s | str_size;
  str.resize(str_size);
  for (auto&& elm : str) {
    s | elm;
  }
}

} /* end namespace serdes */


struct MyCol : Collection<MyCol, Index1D> {
  MyCol() = default;
};


template <typename CallbackT>
struct HelloMsg : vt::Message {
  int from;
  CallbackT cb;

  HelloMsg(int const& in_from, CallbackT cb)
    : Message(), from(in_from), cb(cb)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | from;
    s | cb;
  }
};

struct TestMsg : vt::Message {
  int val = 0;
  std::string s;

  TestMsg() : s("hello") {}

  template <typename SerializerT>
  void serialize(SerializerT& m) {
    m | val;
    m | s;
  }
};
struct TestColMsg : CollectionMessage<MyCol> {
  int val = 0;
};
struct TestMsg2 : vt::Message {
  int val = 0;
};

static void handler(TestColMsg* msg, MyCol* col) {
  fmt::print(
    "{}: triggering collection handler: idx={}\n",
    theContext()->getNode(), col->getIndex()
  );
}

template <typename CallbackT>
static void hello_world(HelloMsg<CallbackT>* msg) {
  fmt::print("{}: Hello from node {}\n", theContext()->getNode(), msg->from);
  auto nmsg = makeSharedMessage<TestMsg>();
  nmsg->val = 2883;
  msg->cb.send(nmsg);
}

template <typename CallbackT>
static void hello_world_2(HelloMsg<CallbackT>* msg) {
  fmt::print("{}: (ANON) Hello from node {}\n", theContext()->getNode(), msg->from);
  auto nmsg = makeSharedMessage<TestMsg>();
  nmsg->val = 2773;
  msg->cb.send(nmsg);
}


template <typename CallbackT>
static void hello_world_3(HelloMsg<CallbackT>* msg) {
  fmt::print("{}: (ANON) Hello from node {}\n", theContext()->getNode(), msg->from);
  msg->cb.send();
}


template <typename CallbackT>
static void hello_world_4(HelloMsg<CallbackT>* msg) {
  fmt::print("{}: (ANON) Hello from node {}\n", theContext()->getNode(), msg->from);
  auto nmsg = makeSharedMessage<TestColMsg>();
  nmsg->val = 2847;
  msg->cb.send(nmsg);
}

static void callback_method(TestMsg* msg) {
  fmt::print("{}: triggering callback_method: {}\n", theContext()->getNode(), msg->s);
}
static void callback_method_2(TestMsg* msg) {
  fmt::print("{}: triggering callback_method_2: {}\n", theContext()->getNode(), msg->s);
}
static void callback_method_3(TestMsg* msg) {
  fmt::print("{}: triggering callback_method_3: {}\n", theContext()->getNode(), msg->s);
}
static void callback_method_4(TestMsg* msg) {
  fmt::print("{}: triggering callback_method_4: {}\n", theContext()->getNode(), msg->s);
}

struct CallbackMethodFunctor {
  void operator()(TestMsg* msg) {
    fmt::print("{}: triggering functor 1\n", theContext()->getNode());
  }
};

struct CallbackMethodFunctorNew {
  void operator()(TestMsg* msg) {
    fmt::print("{}: triggering functor NEW\n", theContext()->getNode());
  }
};

struct CallbackMethodFunctorVoid {
  void operator()() {
    fmt::print("{}: triggering functor VOID\n", theContext()->getNode());
  }
};

struct CallbackMethodFunctorB {
  void operator()(TestMsg* msg) {
    fmt::print("{}: triggering functor 1 bcast\n", theContext()->getNode());
  }
};

struct CallbackMethodFunctorVoidB {
  void operator()() {
    fmt::print("{}: triggering functor VOID bcast\n", theContext()->getNode());
  }
};

#define sstmac_app_name hello_world_vt
int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes == 1) {
    CollectiveOps::abort("At least 2 ranks required");
  }

  if (my_node == 0) {
    auto const dest_x = num_nodes < 4 ? 1 : 2;
    auto cb = theCB()->makeCallbackSingleSendTyped<TestMsg,callback_method>(
      true, dest_x
    );
    // auto cb2 = theCB()->makeCallbackMultiSendTyped<
    //   TestMsg,callback_method,callback_method_2
    // >(true, 2);

    auto cb3a = theCB()->pushTarget<TestMsg,callback_method>(1);
    auto cb3b = theCB()->pushTarget<TestMsg,callback_method>(cb3a,dest_x);
    auto cb3c = theCB()->pushTarget<TestMsg,callback_method_2>(cb3b,1);
    auto cb3d = theCB()->pushTargetBcast<TestMsg,callback_method_3>(cb3c,true);
    auto cb2 = theCB()->buildMultiCB(cb3d);

    //auto cb2 = theCB()->makeCallbackSingleBcast<TestMsg,callback_method>(true);
    //auto cb2 = theCB()->makeCallbackSingleBcastTyped<TestMsg,callback_method>(true);

    auto msg = makeSharedMessage<HelloMsg<decltype(cb2)>>(my_node, cb2);
    theMsg()->sendMsg<HelloMsg<decltype(cb2)>, hello_world>(1, msg);

    {
      auto cbx = theCB()->makeCallbackSingleAnonTyped<TestMsg>(true,[](TestMsg* msg){
        ::fmt::print("A: callback has been triggered! val={}\n", msg->val);
      });
      auto msgx = makeSharedMessage<HelloMsg<decltype(cbx)>>(my_node, cbx);
      theMsg()->sendMsg<HelloMsg<decltype(cbx)>, hello_world_2>(1, msgx);
    }

    {
      auto cbx = theCB()->makeCallbackSingleAnonVoidTyped<>(true,[](){
        ::fmt::print("B: callback has been triggered!\n");
      });
      auto msgx = makeSharedMessage<HelloMsg<decltype(cbx)>>(my_node, cbx);
      theMsg()->sendMsg<HelloMsg<decltype(cbx)>, hello_world_3>(1, msgx);
    }


    // Functor variants
    {
      auto cbx = theCB()->makeCallbackSingleSendFunctorTyped<
        CallbackMethodFunctor
      >(true,dest_x);
      auto msgx = makeSharedMessage<HelloMsg<decltype(cbx)>>(my_node, cbx);
      theMsg()->sendMsg<HelloMsg<decltype(cbx)>, hello_world_2>(1, msgx);
    }

    {
      auto cbx = theCB()->makeCallbackSingleSendFunctorVoidTyped<
        CallbackMethodFunctorVoid
      >(true,dest_x);
      auto msgx = makeSharedMessage<HelloMsg<decltype(cbx)>>(my_node, cbx);
      theMsg()->sendMsg<HelloMsg<decltype(cbx)>, hello_world_3>(1, msgx);
    }

    {
      auto cbx = theCB()->makeCallbackSingleBcastFunctorTyped<
        CallbackMethodFunctorB
      >(true);
      auto msgx = makeSharedMessage<HelloMsg<decltype(cbx)>>(my_node, cbx);
      theMsg()->sendMsg<HelloMsg<decltype(cbx)>, hello_world_2>(1, msgx);
    }

    {
      auto cbx = theCB()->makeCallbackSingleBcastFunctorVoidTyped<
        CallbackMethodFunctorVoidB
      >(true);
      auto msgx = makeSharedMessage<HelloMsg<decltype(cbx)>>(my_node, cbx);
      theMsg()->sendMsg<HelloMsg<decltype(cbx)>, hello_world_3>(1, msgx);
    }


    auto range = Index1D(32);
    auto proxy = theCollection()->construct<MyCol>(range);
    {
      auto cbx = theCB()->makeCallbackSingleProxySendTyped<
        MyCol,TestColMsg,&handler
      >(proxy[10]);
      auto msgx = makeSharedMessage<HelloMsg<decltype(cbx)>>(my_node, cbx);
      theMsg()->sendMsg<HelloMsg<decltype(cbx)>, hello_world_4>(1, msgx);
    }

    /////////////////////////////// typeless ////////////////////////////////

    {
      auto cbx = theCB()->makeCallbackSingleProxySend<
        MyCol,TestColMsg,&handler
      >(proxy[3]);
      auto msgx = makeSharedMessage<HelloMsg<decltype(cbx)>>(my_node, cbx);
      theMsg()->sendMsg<HelloMsg<decltype(cbx)>, hello_world_4>(1, msgx);
    }

    // {
    //   auto cbx = theCB()->makeCallbackSingleProxyBcast<
    //     MyCol,TestColMsg,&handler
    //   >(proxy);
    //   auto msgx = makeSharedMessage<HelloMsg<decltype(cbx)>>(my_node, cbx);
    //   theMsg()->sendMsg<HelloMsg<decltype(cbx)>, hello_world_4>(1, msgx);
    // }

    {
      auto cbx = theCB()->makeCallbackSingleProxyBcastDirect<
        MyCol,TestColMsg,&handler
      >(proxy);
      auto msgx = makeSharedMessage<HelloMsg<decltype(cbx)>>(my_node, cbx);
      theMsg()->sendMsg<HelloMsg<decltype(cbx)>, hello_world_4>(1, msgx);
    }

    // non-proxy
    {
      auto cbx = theCB()->makeCallbackSingleAnon<TestMsg>([](TestMsg* msg){
        ::fmt::print("C: callback has been triggered! val={}\n", msg->val);
      });
      auto msgx = makeSharedMessage<HelloMsg<decltype(cbx)>>(my_node, cbx);
      theMsg()->sendMsg<HelloMsg<decltype(cbx)>, hello_world_2>(1, msgx);
    }

    {
      auto cbx = theCB()->makeCallbackSingleAnonVoid<>([]{
        ::fmt::print("D: callback has been triggered!\n");
      });
      auto msgx = makeSharedMessage<HelloMsg<decltype(cbx)>>(my_node, cbx);
      theMsg()->sendMsg<HelloMsg<decltype(cbx)>, hello_world_3>(1, msgx);
    }


    {
      auto cbx = theCB()->makeCallback<>();
      theCB()->addListener<TestMsg,callback_method_4>(cbx,1);
      theCB()->addListenerFunctor<CallbackMethodFunctorNew>(cbx,dest_x);
      auto msgx = makeSharedMessage<HelloMsg<decltype(cbx)>>(my_node, cbx);
      theMsg()->sendMsg<HelloMsg<decltype(cbx)>, hello_world_2>(1, msgx);
    }

    // {
    //   auto cbx = theCB()->makeCallbackTyped<TestMsg>();
    //   theCB()->addListener<TestMsg,callback_method_4>(cbx,1);
    //   theCB()->addListenerFunctor<CallbackMethodFunctorNew>(cbx,dest_x);
    //   auto msgx = makeSharedMessage<HelloMsg<decltype(cbx)>>(my_node, cbx);
    //   theMsg()->sendMsg<HelloMsg<decltype(cbx)>, hello_world_2>(1, msgx);
    // }

    // Functor variants
    {
      auto cbx = theCB()->makeCallbackFunctorSend<CallbackMethodFunctor>(dest_x);
      auto msgx = makeSharedMessage<HelloMsg<decltype(cbx)>>(my_node, cbx);
      theMsg()->sendMsg<HelloMsg<decltype(cbx)>, hello_world_2>(1, msgx);
    }

    {
      auto cbx = theCB()->makeCallbackFunctorSendVoid<
        CallbackMethodFunctorVoid
      >(dest_x);
      auto msgx = makeSharedMessage<HelloMsg<decltype(cbx)>>(my_node, cbx);
      theMsg()->sendMsg<HelloMsg<decltype(cbx)>, hello_world_3>(1, msgx);
    }

    {
      auto cbx = theCB()->makeCallbackFunctorBcast<CallbackMethodFunctorB>(true);
      auto msgx = makeSharedMessage<HelloMsg<decltype(cbx)>>(my_node, cbx);
      theMsg()->sendMsg<HelloMsg<decltype(cbx)>, hello_world_2>(1, msgx);
    }

    {
      auto cbx = theCB()->makeCallbackFunctorBcastVoid<
        CallbackMethodFunctorVoidB
      >(true);
      auto msgx = makeSharedMessage<HelloMsg<decltype(cbx)>>(my_node, cbx);
      theMsg()->sendMsg<HelloMsg<decltype(cbx)>, hello_world_3>(1, msgx);
    }


    auto cby = theCB()->makeCallbackSingleAnonVoid<>([]{
      ::fmt::print("anon callback has been triggered!\n");
    });

    // New variants
    {
      auto cbx = theCB()->makeSend<CallbackMethodFunctor>(dest_x);
      auto msgx = makeSharedMessage<HelloMsg<decltype(cbx)>>(my_node, cbx);
      theMsg()->sendMsg<HelloMsg<decltype(cbx)>, hello_world_2>(1, msgx);
    }

    {
      auto cbx = theCB()->makeSend<CallbackMethodFunctorVoid>(dest_x);
      auto msgx = makeSharedMessage<HelloMsg<decltype(cbx)>>(my_node, cbx);
      theMsg()->sendMsg<HelloMsg<decltype(cbx)>, hello_world_3>(1, msgx);
    }

    {
      auto cbx = theCB()->makeBcast<CallbackMethodFunctorB>();
      auto msgx = makeSharedMessage<HelloMsg<decltype(cbx)>>(my_node, cbx);
      theMsg()->sendMsg<HelloMsg<decltype(cbx)>, hello_world_2>(1, msgx);
    }

    {
      auto cbx = theCB()->makeBcast<CallbackMethodFunctorVoidB>();
      auto msgx = makeSharedMessage<HelloMsg<decltype(cbx)>>(my_node, cbx);
      theMsg()->sendMsg<HelloMsg<decltype(cbx)>, hello_world_3>(1, msgx);
    }

  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
