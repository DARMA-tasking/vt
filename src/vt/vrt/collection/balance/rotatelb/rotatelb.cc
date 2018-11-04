
#include "vt/config.h"
#include "vt/vrt/collection/balance/rotatelb/rotatelb.h"
#include "vt/vrt/collection/manager.h"

#include <memory>

namespace vt { namespace vrt { namespace collection { namespace lb {

void RotateLB::procDataIn(ElementLoadType const& data_in) {
  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();
  auto const next_node = this_node + 1 > num_nodes-1 ? 0 : this_node + 1;
  if (this_node == 0) {
    fmt::print(
      "VT: {}: "
      "RotateLB: procDataIn: stats size={}, next_node={}\n",
      this_node, data_in.size(), next_node
    );
    fflush(stdout);
  }
  debug_print(
    lblite, node,
    "RotateLB::procDataIn: size={}, next_node={}\n",
    data_in.size(), next_node
  );
  EpochType const epoch = theTerm()->newEpoch();
  theMsg()->setGlobalEpoch(epoch);
  theTerm()->addActionEpoch(epoch,[this]{ this->finishedMigrate(); });
  for (auto&& stat : data_in) {
    auto const& obj = stat.first;
    auto const& load = stat.second;
    debug_print(
      lblite, node,
      "\t RotateLB::procDataIn: obj={}, load={}\n",
      obj, load
    );
    auto iter = balance::ProcStats::proc_migrate_.find(obj);
    assert(iter != balance::ProcStats::proc_migrate_.end() && "Must exist");
    transfer_count++;
    iter->second(next_node);
  }
  theTerm()->finishedEpoch(epoch);
}

void RotateLB::finishedMigrate() {
  debug_print(
    lblite, node,
    "RotateLB::finishedMigrate: transfer_count={}\n",
    transfer_count
  );
  theMsg()->setGlobalEpoch();
  balance::ProcStats::proc_migrate_.clear();
  balance::ProcStats::proc_data_.clear();
  balance::ProcStats::next_elm_ = 1;
  theCollection()->releaseLBContinuation();
}

/*static*/ void RotateLB::rotateLBHandler(balance::RotateLBMsg* msg) {
  auto const& phase = msg->getPhase();
  RotateLB::rotate_lb_inst = std::make_unique<RotateLB>();
  assert(balance::ProcStats::proc_data_.size() >= phase);
  debug_print(
    lblite, node,
    "\t RotateLB::rotateLBHandler: phase={}\n", phase
  );
  RotateLB::rotate_lb_inst->procDataIn(balance::ProcStats::proc_data_[phase]);
}

/*static*/ std::unique_ptr<RotateLB> RotateLB::rotate_lb_inst;

}}}} /* end namespace vt::vrt::collection::lb */

