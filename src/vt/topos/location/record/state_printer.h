
#if !defined INCLUDED_TOPOS_LOCATION_RECORD_STATE_PRINTER_H
#define INCLUDED_TOPOS_LOCATION_RECORD_STATE_PRINTER_H

#include "vt/config.h"
#include "vt/topos/location/record/state.h"
#include "vt/topos/location/record/state_stringize.h"

#include <cstdlib>
#include <cstdint>
#include <iostream>

namespace vt { namespace location {

template <typename EntityID>
std::ostream& operator<<(std::ostream& os, LocRecord<EntityID> const& rec) {
  auto state_val = (int32_t) rec.state_;
  os << "id=" << rec.id_ << ", "
     << "state=" << rec.state_ << ", "
     << "state=" << state_val << ", "
     << "cur_node_=" << rec.cur_node_;
  return os;
}

inline std::ostream& operator<<(std::ostream& os, eLocState const& state) {
  os << print_location_record_state(state);
  return os;
}

}}  // end namespace vt::location

#endif /*INCLUDED_TOPOS_LOCATION_RECORD_STATE_PRINTER_H*/
