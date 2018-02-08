
#include "config.h"
#include "topos/location/location_common.h"
#include "topos/location/manager.h"

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

/*virtual*/ LocationManager::~LocationManager() {
  virtual_loc = nullptr;
  vrtContextLoc = nullptr;
}

using LocType = LocationManager;

/*static*/ LocType::PtrType<LocType::VrtLocType> LocType::virtual_loc =
  std::make_unique<LocType::VrtLocType>();

/*static*/ LocType::PtrType<LocType::VrtLocProxyType> LocType::vrtContextLoc =
  std::make_unique<LocType::VrtLocProxyType>();

}}  // end namespace vt::location
