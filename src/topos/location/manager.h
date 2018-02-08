
#if !defined INCLUDED_TOPOS_LOCATION_MANAGER_H
#define INCLUDED_TOPOS_LOCATION_MANAGER_H

#include "config.h"
#include "topos/location/location_common.h"
#include "topos/location/location.fwd.h"
#include "topos/location/utility/coord.h"
#include "vrt/vrt_common.h"

namespace vt { namespace location {

struct LocationManager {
  template <typename LocType>
  using PtrType = std::unique_ptr<LocType>;
  using LocCoordPtrType = LocationCoord*;
  using LocInstContainerType = std::vector<LocCoordPtrType>;
  using VrtLocType = EntityLocationCoord<int32_t>;
  using VrtLocProxyType = EntityLocationCoord<VirtualProxyType>;

  template <typename IndexT>
  using CollectionProxyType = ::vt::vrt::VirtualElmProxyType<IndexT>;
  template <typename IndexT>
  using VrtColl = EntityLocationCoord<CollectionProxyType<IndexT>>;

  // Different types of location managed entities
  template <typename IndexT>
  static PtrType<VrtColl<IndexT>> collectionLoc;
  static PtrType<VrtLocType> virtual_loc;
  static PtrType<VrtLocProxyType> vrtContextLoc;

  // Manage different instances of individually managed entities
  static void insertInstance(int const i, LocCoordPtrType const& ptr);
  static LocCoordPtrType getInstance(int const inst);

  virtual ~LocationManager();

 private:
  static LocInstContainerType loc_insts;
};

}} /* end namespace vt::location */

#include "topos/location/manager.impl.h"

namespace vt {

extern location::LocationManager* theLocMan();

}  // end namespace vt

#endif /*INCLUDED_TOPOS_LOCATION_MANAGER_H*/
