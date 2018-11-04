
#include "vt/config.h"
#include "vt/lb/lb_types.h"
#include "vt/lb/instrumentation/database.h"
#include "vt/lb/instrumentation/entry.h"

namespace vt { namespace lb { namespace instrumentation {

void Database::addEntry(EntryType&& entry) {
  auto const& phase = cur_phase_;
  auto phase_iter = phase_timings_.find(phase);
  if (phase_iter != phase_timings_.end()) {
    phase_timings_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(phase),
      std::forward_as_tuple(EntryListType{std::move(entry)})
    );
  } else {
    phase_iter->second.emplace_back(std::move(entry));
  }
}

}}} /* end namespace vt::lb::instrumentation */
