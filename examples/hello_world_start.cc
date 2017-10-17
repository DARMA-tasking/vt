
#include "transport.h"
#include <cstdlib>
#include <cstdio>

using namespace vt;
using namespace vt::vrt;
using namespace vt::mapping;

struct ProxyMsg : vt::vrt::VirtualMessage {
  std::vector<VirtualProxyType> proxies;

  ProxyMsg() = default;
  ProxyMsg(std::vector<VirtualProxyType> const& in_proxies)
    : VirtualMessage(), proxies(in_proxies)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    VirtualMessage::serialize(s);
    s | proxies;
  }
};

struct TestVC : vt::vrt::VirtualContext {
  int my_index = 0;
  int work_amt = 0;

  TestVC(int const in_my_index, int const in_work_amt)
    : my_index(in_my_index), work_amt(in_work_amt)
  {
    printf(
      "constructing TestVC: my_index=%d, work amt=%d\n", my_index, work_amt
    );
  }

  void getRightProxy(ProxyMsg* msg) {
    auto my_proxy = msg->proxies[my_index];
    assert(my_proxy == getProxy());

    auto const to_right = my_index+1;
    printf("myproxy=%lld %ld right=%d vc=%p\n", getProxy(), msg->proxies.size(), to_right, this);
    if (msg->proxies.size()-1 < to_right) {

    }
  }
};

static void proxyHan(ProxyMsg* msg, TestVC* vc) {
  printf("proxyHan: msg->proxies=%ld, vc=%p\n", msg->proxies.size(), vc);
  vc->getRightProxy(msg);
}

struct MainVC : vt::vrt::MainVirtualContext {
  int my_data = -1;

  MainVC() : my_data(4) {
    printf("constructing MainVC: data=%d\n", my_data);

    std::vector<VirtualProxyType> proxies;

    for (auto i = 0; i < my_data; i++) {
      auto proxy = theVirtualManager->makeVirtualMap<TestVC, randomSeedMapCore>(
        i, i * 82773
      );
      proxies.push_back(proxy);
    }

    for (auto&& elm : proxies) {
      ProxyMsg* msg = makeSharedMessage<ProxyMsg>(proxies);
      printf("sending to proxy %lld\n", elm);
      theVirtualManager->sendSerialMsg<TestVC, ProxyMsg, proxyHan>(elm, msg);
    }
  }
};

VT_REGISTER_MAIN_CONTEXT(MainVC);
