
#include "config.h"
#include "topos/location/location.h"

#include <cassert>

namespace vt { namespace location {

/*static*/ LocationManager::LocInstContainerType LocationManager::loc_insts;

/*static*/ void LocationManager::insertInstance(
  int const inst, LocCoordPtrType const& ptr
) {
  if (loc_insts.size() < inst + 1) {
    loc_insts.resize(inst + 1);
  }
  loc_insts.at(inst) = ptr;
}

/*static*/ LocationManager::LocCoordPtrType LocationManager::getInstance(
  int const inst
) {
  assert(loc_insts.size() > inst and "inst must exist in container");
  return loc_insts.at(inst);
}

}}  // end namespace vt::location
