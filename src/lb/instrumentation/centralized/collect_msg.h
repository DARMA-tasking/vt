
#if !defined INCLUDED_LB_INSTRUMENTATION_CENTRALIZED_COLLECT_MSG_H
#define INCLUDED_LB_INSTRUMENTATION_CENTRALIZED_COLLECT_MSG_H

#include "config.h"
#include "lb/lb_types.h"
#include "lb/instrumentation/entry.h"
#include "messaging/message.h"

namespace vt { namespace lb { namespace instrumentation {

struct CollectMsg : ::vt::Message {
  CollectMsg() = default;

  explicit CollectMsg(Database::EntryListType const& in_phase_entries)
    : phase_entries_(in_phase_entries)
  { }

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | phase_entries_;
  }

public:
  Database::EntryListType phase_entries_;
};

}}} /* end namespace vt::lb::instrumentation */

#endif /*INCLUDED_LB_INSTRUMENTATION_CENTRALIZED_COLLECT_MSG_H*/
