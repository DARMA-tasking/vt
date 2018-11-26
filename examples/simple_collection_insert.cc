
#include "vt/transport.h"

#include <cstdlib>
#include <cassert>

using namespace ::vt;
using namespace ::vt::collective;
using namespace ::vt::mapping;

static constexpr std::size_t const default_num_elms = 64;

using IndexType = IdxType1D<std::size_t>;

struct TestColl : Collection<TestColl,IndexType> {
  TestColl() = default;

  virtual ~TestColl() {
    // auto num_nodes = theContext()->getNumNodes();
    // vtAssertInfo(
    //   counter_ == num_nodes, "Must be equal",
    //   counter_, num_nodes, getIndex(), theContext()->getNode()
    // );
    fmt::print("{}: destroying TestColl\n", getIndex());
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

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  int32_t num_elms = default_num_elms;

  if (argc > 1) {
    num_elms = atoi(argv[1]);
  }

  using BaseIndexType = typename IndexType::DenseIndexType;
  auto const& range = IndexType(static_cast<BaseIndexType>(num_elms));
  auto token = theCollection()->constructInsert<TestColl>(range);

  for (auto i = 0; i < default_num_elms; i++) {
    if (i % num_nodes == this_node) {
      fmt::print("node={}: inserting {}\n", this_node, i);
      token[i].insert();
    }
  }

  fmt::print("finishedInsert: node={}: finished\n", this_node);
  auto proxy = theCollection()->finishedInsert(std::move(token));

  if (this_node == 1) {
    auto msg = makeSharedMessage<TestColl::TestMsg>();
    proxy[2].send<TestColl::TestMsg,&TestColl::doWork>(msg);
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
