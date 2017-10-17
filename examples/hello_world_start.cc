
#include "transport.h"

#include <cstdlib>
#include <cstdio>
#include <vector>

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

struct WorkMsg : vt::vrt::VirtualMessage {
  std::vector<double> work_vec;

  WorkMsg() = default;

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    VirtualMessage::serialize(s);
    s | work_vec;
  }
};

struct TestVC;
static void doWorkRight(WorkMsg* msg, TestVC* vc);

struct TestVC : vt::vrt::VirtualContext {
  VirtualProxyType my_proxy = no_vrt_proxy;
  VirtualProxyType right_proxy = no_vrt_proxy;
  int my_index = 0;
  int work_amt = 0;

  double stored_val = 0.0;

  TestVC(int const in_my_index, int const in_work_amt)
    : my_index(in_my_index), work_amt(in_work_amt)
  {
    printf(
      "constructing TestVC: my_index=%d, work amt=%d\n", my_index, work_amt
    );
  }

  void getRightProxy(ProxyMsg* msg) {
    my_proxy = msg->proxies[my_index];
    assert(my_proxy == getProxy());

    auto const to_right = my_index+1;
    bool const has_right = to_right < msg->proxies.size();

    if (has_right) {
      right_proxy = msg->proxies[to_right];
    }

    printf(
      "my_proxy=%lld, proxies size=%ld, right=%d vc=%p has_right=%s, "
      "right proxy=%lld\n",
      getProxy(), msg->proxies.size(), to_right, this, print_bool(has_right),
      right_proxy
    );

    if (has_right) {
      auto msg = makeSharedMessage<WorkMsg>();
      for (auto i = 0; i < my_index + 29; i++) {
        msg->work_vec.push_back(static_cast<double>(work_amt));
      }
      theVirtualManager->sendSerialMsg<TestVC, WorkMsg, doWorkRight>(
        right_proxy, msg
      );
    }
  }

  void doWork(WorkMsg* msg) {
    printf(
      "my_proxy=%lld, doing work size=%ld\n", my_proxy, msg->work_vec.size()
    );

    double val = 1.23;
    for (auto&& elm : msg->work_vec) {
      val += elm * 3.14159;
    }
    stored_val = val;

    printf(
      "my_proxy=%lld, finished work val=%f\n", my_proxy, stored_val
    );
  }
};

static void doWorkRight(WorkMsg* msg, TestVC* vc) {
  printf(
    "doWorkRight: msg->work_vec.size()=%ld, vc=%p\n", msg->work_vec.size(), vc
  );
  vc->doWork(msg);
}

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
