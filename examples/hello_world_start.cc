
#include "transport.h"
#include "utils/tls/tls.h"

#include <cstdlib>
#include <cstdio>
#include <vector>

using namespace vt;
using namespace vt::vrt;
using namespace vt::mapping;

#define DEBUG_START_EXAMPLE 0

#define DEBUG_PRINTER_START(str, args...)                               \
  do{                                                                   \
    fmt::print(                                                         \
      "node={},worker={}: " str,                                        \
      theContext()->getNode(), theContext()->getWorker(), args          \
    );                                                                  \
  } while (false);

#if DEBUG_START_EXAMPLE
  #define DEBUG_PRINT_START(str, args...) DEBUG_PRINTER_START(str, args)
#else
  #define DEBUG_PRINT_START(str, args...)
#endif

struct ProxyMsg : vt::vrt::VirtualMessage {
  std::vector<VirtualProxyType> proxies;

  ProxyMsg() = default;
  ProxyMsg(std::vector<VirtualProxyType> const& in_proxies)
    : VirtualMessage(), proxies(in_proxies)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | proxies;
  }
};

struct WorkMsg : vt::vrt::VirtualMessage {
  std::vector<double> work_vec;

  WorkMsg() = default;

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | work_vec;
  }
};

struct TestVC;
static void doWorkRight(WorkMsg* msg, TestVC* vc);

static ::vt::util::atomic::AtomicType<int> num_work_finished = {0};

DeclareInitTLS(int, tls_work, 0);

struct TestVC : vt::vrt::VirtualContext {
  VirtualProxyType my_proxy = no_vrt_proxy;
  VirtualProxyType right_proxy = no_vrt_proxy;
  int my_index = 0;
  int work_amt = 0;

  double stored_val = 0.0;

  TestVC(int const in_my_index, int const in_work_amt)
    : my_index(in_my_index), work_amt(in_work_amt)
  {
    DEBUG_PRINT_START(
      "node={}, worker={}: {}: constructing TestVC: my_index={}, work amt={}\n",
      theContext()->getNode(), theContext()->getWorker(), this, my_index,
      work_amt
    );
  }

  void getRightProxy(ProxyMsg* msg) {
    my_proxy = msg->proxies[my_index];

    DEBUG_PRINT_START(
      "my_proxy={}, index={}, getProxy()={}, ptr={}\n",
      my_proxy, my_index, getProxy(), this
    );

    vtAssertExpr(my_proxy == getProxy());

    auto const to_right = my_index+1;
    bool const has_right = to_right < msg->proxies.size();

    if (has_right) {
      right_proxy = msg->proxies[to_right];
    }

    DEBUG_PRINT_START(
      "my_proxy={}, proxies size={}, right={} vc={} has_right={}, "
      "right proxy={}\n",
      getProxy(), msg->proxies.size(), to_right, this, print_bool(has_right),
      right_proxy
    );

    if (has_right) {
      auto msg = makeSharedMessage<WorkMsg>();
      for (auto i = 0; i < my_index + 29; i++) {
        msg->work_vec.push_back(static_cast<double>(work_amt));
      }

      theVirtualManager()->sendSerialMsg<TestVC, WorkMsg, doWorkRight>(
        right_proxy, msg
      );
    }
  }

  void doWork(WorkMsg* msg) {
    DEBUG_PRINT_START(
      "my_proxy={}, doing work size={}\n", my_proxy, msg->work_vec.size()
    );

    double val = 1.23;
    for (auto&& elm : msg->work_vec) {
      val += elm * 3.14159;
    }
    stored_val = val;

    num_work_finished++;
    AccessTLS(tls_work)++;

    DEBUG_PRINT_START(
      "my_proxy={}, finished work val={}\n", my_proxy, stored_val
    );
  }
};

static void doWorkRight(WorkMsg* msg, TestVC* vc) {
  DEBUG_PRINT_START(
    "doWorkRight: msg->work_vec.size()={}, vc={}\n", msg->work_vec.size(), vc
  );
  vc->doWork(msg);
}

static void proxyHan(ProxyMsg* msg, TestVC* vc) {
  DEBUG_PRINT_START("proxyHan: msg->proxies={}, vc={}\n", msg->proxies.size(), vc);
  vc->getRightProxy(msg);
}

struct MakeMainMsg : vt::Message { };
static void makeMain(MakeMainMsg* msg);

struct MainVC : vt::vrt::MainVirtualContext {
  int my_data = -1;

  MainVC() : my_data(29) {
    DEBUG_PRINT_START("constructing MainVC: data={}\n", my_data);

    if (theContext()->getNode() == 0) {
      auto msg = makeSharedMessage<MakeMainMsg>();
      theMsg()->broadcastMsg<MakeMainMsg, makeMain>(msg);
    }

    theTerm()->addAction([]{
      auto const num_work_units = num_work_finished.load();
      DEBUG_PRINTER_START("num_work_units={}\n", num_work_units);

      // theWorkerGrp()->enqueueAllWorkers([]{
      //   auto const num_work_units = AccessTLS(tls_work);
      //   DEBUG_PRINTER_START("tls work_units={}\n", num_work_units);
      // });
    });

    std::vector<VirtualProxyType> proxies;

    for (auto i = 0; i < my_data; i++) {
      //auto proxy = theVirtualManager()->makeVirtual<TestVC>(i, i * 82773);
      auto proxy = theVirtualManager()->makeVirtualMap<TestVC, randomSeedMapCore>(
        i, i * 82773
      );
      proxies.push_back(proxy);

      DEBUG_PRINT_START(
        "{}: constructing proxy {}, size={}, work={}\n",
        i, proxy, proxies.size(), i*82773
      );
    }

    int i = 0;
    for (auto&& elm : proxies) {
      ProxyMsg* msg = makeSharedMessage<ProxyMsg>(proxies);
      DEBUG_PRINT_START("{}: sending to proxy {}\n", i, elm);
      theVirtualManager()->sendSerialMsg<TestVC, ProxyMsg, proxyHan>(elm, msg);
      i++;
    }
  }
};

static void makeMain(MakeMainMsg* msg) {
  theVirtualManager()->makeVirtual<MainVC>();
}


VT_REGISTER_MAIN_CONTEXT(MainVC);
