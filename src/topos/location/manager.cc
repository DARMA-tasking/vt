
#include "config.h"
#include "topos/location/location_common.h"
#include "topos/location/manager.h"

#include <cassert>

namespace vt { namespace location {

/*static*/ LocationManager::LocInstContainerType LocationManager::loc_insts;

/*static*/ LocationManager::LocCoordPtrType LocationManager::getInstance(
  LocInstType const inst
) {
  assert(
    loc_insts.size() > inst &&
    "LocationManager instance must exist in container"
  );

  return loc_insts.at(inst);
}

/*virtual*/ LocationManager::~LocationManager() {
  virtual_loc = nullptr;
  vrtContextLoc = nullptr;
  collectionLoc.clear();
}

/*static*/ LocInstType LocationManager::cur_loc_inst = 0xFF00000000000000;

}}  // end namespace vt::location
