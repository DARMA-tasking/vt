
#include "config.h"
#include "vrt/collection/balance/proc_stats.h"
#include "vrt/collection/manager.h"
#include "timing/timing.h"

#include <vector>
#include <unordered_map>


namespace vt { namespace vrt { namespace collection { namespace balance {

/*static*/
std::vector<
  std::unordered_map<ProcStats::ElementIDType,TimeType>
> ProcStats::proc_data_ = {};

/*static*/
std::unordered_map<ProcStats::ElementIDType,ProcStats::MigrateFnType>
  ProcStats::proc_migrate_ = {};

/*static*/ ProcStats::ElementIDType ProcStats::next_elm_ = 1;

/*static*/ void ProcStats::clearStats() {
  ProcStats::proc_data_.clear();
  ProcStats::proc_migrate_.clear();
  next_elm_ = 1;
}

/*static*/ ProcStats::ElementIDType ProcStats::getNextElm() {
  auto const& this_node = theContext()->getNode();
  auto elm = next_elm_++;
  return (elm << 32) | this_node;
}

/*static*/ void ProcStats::releaseLB() {
  auto msg = makeSharedMessage<CollectionPhaseMsg>();
  theMsg()->broadcastMsg<
    CollectionPhaseMsg,CollectionManager::releaseLBPhase
  >(msg);
  auto msg_this = makeSharedMessage<CollectionPhaseMsg>();
  messageRef(msg_this);
  CollectionManager::releaseLBPhase(msg_this);
  messageDeref(msg_this);
}

}}}} /* end namespace vt::vrt::collection::balance */
