
#include "vt/transport.h"

#include <cstdlib>
#include <cassert>

using namespace ::vt;
using namespace ::vt::collective;
using namespace ::vt::mapping;

static constexpr std::size_t const default_num_elms = 8;

using TestReduceMsg = ReduceTMsg<int>;

struct TestColl : Collection<TestColl,Index1D> {
  TestColl() = default;

  virtual ~TestColl() = default;

  struct TestMsg : CollectionMessage<TestColl> { };

  void done(TestReduceMsg* msg) {
    auto const& this_node = theContext()->getNode();
    ::fmt::print("{}: done: idx={}, val={}\n", this_node, getIndex().x(), msg->getVal());
  }

  void doWork(TestMsg* msg) {
    auto const& this_node = theContext()->getNode();
    ::fmt::print("{}: doWork: idx={}\n", this_node, getIndex().x());

    auto proxy = this->getCollectionProxy();
    auto cb = theCB()->makeSend<TestColl,TestReduceMsg,&TestColl::done>(proxy(2));

    vtAssertExpr(cb.valid());
    auto rmsg = makeMessage<TestReduceMsg>();
    rmsg->getVal() = 10;
    proxy.reduce<collective::PlusOp<int>>(rmsg.get(),cb);
  }

private:
  int32_t counter_ = 0;
};

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  int32_t num_elms = default_num_elms;

  if (argc > 1) {
    num_elms = atoi(argv[1]);
  }

  if (this_node == 0) {
    using IndexType = typename TestColl::IndexType;
    using BaseIndexType = typename IndexType::DenseIndexType;
    auto const& range = IndexType(static_cast<BaseIndexType>(num_elms));
    auto proxy = theCollection()->construct<TestColl>(range);

    auto msg = makeSharedMessage<TestColl::TestMsg>();
    proxy.broadcast<TestColl::TestMsg,&TestColl::doWork>(msg);
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
