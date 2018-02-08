
#if !defined INCLUDED_TOPOS_LOCATION_MANAGER_IMPL_H
#define INCLUDED_TOPOS_LOCATION_MANAGER_IMPL_H

#include "config.h"
#include "topos/location/manager.h"
#include "topos/location/location_common.h"
#include "topos/location/location.h"

#include <memory>

namespace vt { namespace location {

using LocType = LocationManager;

template <typename IndexT>
/*static*/ LocType::PtrType<LocType::VrtColl<IndexT>> LocType::collectionLoc =
  std::make_unique<LocType::VrtColl<IndexT>>();

}} /* end namespace vt::location */

#endif /*INCLUDED_TOPOS_LOCATION_MANAGER_IMPL_H*/
