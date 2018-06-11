
#include "transport.h"

#include <cstdlib>

using namespace vt;
using namespace vt::vrt;
using namespace vt::vrt::collection;
using namespace vt::index;
using namespace vt::mapping;

static constexpr int32_t const default_num_elms = 16;
static constexpr int32_t const num_iter = 8;

struct IterCol : Collection<IterCol,Index1D> {
  IterCol() = default;

private:
  int data_1 = 29;
public:
  float data_2 = 2.4f;
};

struct IterMsg : CollectionMessage<IterCol> {
  IterMsg() = default;
  explicit IterMsg(int64_t const in_work_amt, int64_t const in_iter)
    : iter_(in_iter), work_amt_(in_work_amt)
  { }

  int64_t iter_ = 0;
  int64_t work_amt_ = 0;
};

static void iterWork(IterMsg* msg, IterCol* col) {
  double val = 0.1f;
  double val2 = 0.4f * msg->work_amt_;
  auto const idx = col->getIndex().x();
  auto const iter = msg->iter_;
  ::fmt::print("idx={}, iter={}\n", idx, iter);
  int const x = idx < 8 ? 10000 : (idx > 40 ? 1000 : 10);
  for (int i = 0; i < 10000 * x; i++) {
    val *= val2 + i*29.4;
    val2 += 1.0;
  }
  col->data_2 += val + val2;
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
    auto proxy = theCollection()->construct<IterCol>(range);

    for (auto i = 0; i < num_iter; i++) {
      auto msg = new IterMsg(10,i);
      proxy.broadcast<IterMsg,iterWork>(msg);
    }
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
