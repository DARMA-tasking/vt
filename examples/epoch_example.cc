
#include "vt/transport.h"

#include <cstdlib>

using namespace vt;

static NodeType my_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  my_node = theContext()->getNode();
  num_nodes = theContext()->getNumNodes();

  {
    EpochType seq = 0xFFAA;
    auto const rooted = epoch::EpochManip::makeEpoch(seq, true, my_node, false);
    auto const is_rooted = epoch::EpochManip::isRooted(rooted);
    auto const is_user = epoch::EpochManip::isUser(rooted);
    auto const has_category = epoch::EpochManip::hasCategory(rooted);
    auto const get_seq = epoch::EpochManip::seq(rooted);
    auto const ep_node = epoch::EpochManip::node(rooted);
    auto const next = epoch::EpochManip::next(rooted);
    auto const next_seq = epoch::EpochManip::seq(next);

    ::fmt::print(
      "rooted epoch={}, is_rooted={}, has_cat={}, is_user={}, get_seq={}, "
      "node={}, next={}, next_seq={}, num={}, end={}\n",
      rooted, is_rooted, has_category, is_user, get_seq, ep_node, next,
      next_seq, epoch::epoch_seq_num_bits, epoch::eEpochLayout::EpochSentinelEnd
    );
    ::fmt::print(
      "epoch={}, seq={}\n", rooted, get_seq
    );
    printf("epoch %llu , %llx : seq %llu , %llx\n", rooted, rooted, get_seq, get_seq);
    printf("epoch %llu , %llx : seq %llu , %llx\n", next, next, next_seq, next_seq);
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
