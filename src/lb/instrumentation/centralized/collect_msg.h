
#if !defined INCLUDED_LB_INSTRUMENTATION_CENTRALIZED_COLLECT_MSG_H
#define INCLUDED_LB_INSTRUMENTATION_CENTRALIZED_COLLECT_MSG_H

#include "config.h"
#include "lb/lb_types.h"
#include "lb/instrumentation/entry.h"
#include "collective/reduce/reduce.h"
#include "messaging/message.h"

#include <vector>
#include <unordered_map>

namespace vt { namespace lb { namespace instrumentation {

struct CollectMsg : ::vt::collective::reduce::ReduceMsg {
  using EntryType = Entry;
  using EntryListType = std::vector<EntryType>;
  using ContainerType = std::unordered_map<LBEntityType, EntryListType>;
  using ProcContainerType = std::unordered_map<NodeType, ContainerType>;

  CollectMsg() = default;

  CollectMsg(
    LBPhaseType const& in_phase, ProcContainerType const& in_entries
  ) : phase_(in_phase), entries_(in_entries)
  { }
  explicit CollectMsg(LBPhaseType const& in_phase)
    : CollectMsg(in_phase, ProcContainerType{})
  { }

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | entries_;
    s | phase_;
  }

public:
  LBPhaseType phase_ = no_phase;
  ProcContainerType entries_;
};

}}} /* end namespace vt::lb::instrumentation */

#endif /*INCLUDED_LB_INSTRUMENTATION_CENTRALIZED_COLLECT_MSG_H*/
