
#include "vt/transport.h"

#include <cstdlib>
#include <cassert>

using namespace ::vt;
using namespace ::vt::collective;
using namespace ::vt::index;
using namespace ::vt::mapping;

static constexpr int32_t const default_num_elms = 64;

struct TestColl : Collection<TestColl,Index1D> {
  TestColl() = default;

  virtual ~TestColl() {
    auto num_nodes = theContext()->getNumNodes();
    vtAssertInfo(
      counter_ == num_nodes, "Must be equal",
      counter_, num_nodes, getIndex(), theContext()->getNode()
    );
  }

  struct TestMsg : CollectionMessage<TestColl> { };

  void doWork(TestMsg* msg) {
    auto const& this_node = theContext()->getNode();
    counter_++;
    ::fmt::print(
      "{}: doWork: idx={}, cnt={}\n", this_node, getIndex().x(), counter_
    );
  }

private:
  int32_t counter_ = 0;
};

template <typename ProxyT>
struct ProxyMsg : Message {
  ProxyMsg() = default;
  explicit ProxyMsg(ProxyT const& in_proxy) : proxy_(in_proxy) { }
  ProxyT proxy_ = {};
};

template <typename ProxyMsgT>
static void proxyHandler(ProxyMsgT* msg) {
  auto msg_send = makeSharedMessage<TestColl::TestMsg>();
  msg->proxy_.template broadcast<TestColl::TestMsg,&TestColl::doWork>(msg_send);
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  int32_t num_elms = default_num_elms;

  if (argc > 1) {
    num_elms = atoi(argv[1]);
  }

  if (this_node == 0) {
    auto const& range = Index1D(num_elms);
    auto proxy = theCollection()->construct<TestColl>(range);

    auto msg = makeSharedMessage<TestColl::TestMsg>();
    proxy.broadcast<TestColl::TestMsg,&TestColl::doWork>(msg);

    using ProxyMsgType = ProxyMsg<decltype(proxy)>;
    auto proxy_msg = makeSharedMessage<ProxyMsgType>(proxy);
    theMsg()->broadcastMsg<ProxyMsgType,proxyHandler<ProxyMsgType>>(proxy_msg);
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
