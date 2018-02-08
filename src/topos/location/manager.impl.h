
#if !defined INCLUDED_TOPOS_LOCATION_MANAGER_IMPL_H
#define INCLUDED_TOPOS_LOCATION_MANAGER_IMPL_H

#include "config.h"
#include "topos/location/location_common.h"
#include "topos/location/manager.h"

#include <memory>

namespace vt { namespace location {

using LocManType = LocationManager;

/*static*/ LocManType::PtrType<LocManType::VrtLocType> virtual_loc =
  std::make_unique<LocManType::VrtLocType>();

/*static*/ LocManType::PtrType<LocManType::VrtLocProxyType> vrtContextLoc =
  std::make_unique<LocManType::VrtLocProxyType>();

template <typename IndexT>
/*static*/ LocManType::PtrType<LocManType::VrtColl<IndexT>> collectionLoc =
  std::make_unique<LocManType::VrtColl<IndexT>>();

}} /* end namespace vt::location */

#endif /*INCLUDED_TOPOS_LOCATION_MANAGER_IMPL_H*/
