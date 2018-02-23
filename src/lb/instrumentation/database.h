
#if !defined INCLUDED_LB_INSTRUMENTATION_DATABASE_H
#define INCLUDED_LB_INSTRUMENTATION_DATABASE_H

#include "config.h"
#include "lb/lb_types.h"
#include "lb/instrumentation/entry.h"

#include <unordered_map>
#include <vector>

namespace vt { namespace lb { namespace instrumentation {

struct Database {
  using EntryType = Entry;
  using EntryListType = std::vector<EntryType>;

  Database() = default;

  void addEntry(EntryType&& entry);

private:
  // Past timings ordered by time ascending
  std::unordered_map<LBPhaseType, EntryListType> phase_timings_;

  // Current/next phase for database entry
  LBPhaseType cur_phase_ = 0;
};

}}} /* end namespace vt::lb::instrumentation */

#endif /*INCLUDED_LB_INSTRUMENTATION_DATABASE_H*/
