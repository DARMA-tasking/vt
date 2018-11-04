
#include "vt/config.h"
#include "vt/topos/location/location_common.h"
#include "vt/topos/location/manager.h"

#include <cassert>

namespace vt { namespace location {

/*static*/ LocationManager::LocInstContainerType LocationManager::loc_insts;

/*static*/ LocationManager::LocCoordPtrType LocationManager::getInstance(
  LocInstType const inst
) {
  auto inst_iter = loc_insts.find(inst);

  assert(
    inst_iter != loc_insts.end() &&
    "LocationManager instance must exist in container"
  );

  return inst_iter->second;
}

/*virtual*/ LocationManager::~LocationManager() {
  virtual_loc = nullptr;
  vrtContextLoc = nullptr;
  collectionLoc.clear();
  loc_insts.clear();
  //pending_inst_.clear();
  cur_loc_inst = 0xFF00000000000000;
}

/*static*/ LocInstType LocationManager::cur_loc_inst = 0xFF00000000000000;

}}  // end namespace vt::location
