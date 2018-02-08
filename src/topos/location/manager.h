
#if !defined INCLUDED_TOPOS_LOCATION_MANAGER_H
#define INCLUDED_TOPOS_LOCATION_MANAGER_H

#include "config.h"
#include "topos/location/location_common.h"
#include "topos/location/location.fwd.h"
#include "topos/location/utility/coord.h"
#include "vrt/vrt_common.h"

#include <unordered_map>
#include <functional>

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
  template <typename IndexT>
  using CollectionLocType = PtrType<VrtColl<IndexT>>;
  using LocErasureType = LocationCoord;
  using LocDeleterType = std::function<void(LocErasureType*)>;
  using CollectionLocErasedType = std::unique_ptr<
    LocErasureType, LocDeleterType
  >;
  using CollectionContainerType = std::unordered_map<
    VirtualProxyType, CollectionLocErasedType
  >;

  LocationManager() = default;

  virtual ~LocationManager();

  PtrType<VrtLocType> virtual_loc = std::make_unique<VrtLocType>();;
  PtrType<VrtLocProxyType> vrtContextLoc = std::make_unique<VrtLocProxyType>();

  template <typename IndexT>
  VrtColl<IndexT>* getCollectionLM(VirtualProxyType const& proxy);

public:
  // Manage different instances of individually managed entities
  static void insertInstance(int const i, LocCoordPtrType const& ptr);
  static LocCoordPtrType getInstance(int const inst);

protected:
  CollectionContainerType collectionLoc;

 private:
  static LocInstContainerType loc_insts;
};

}} /* end namespace vt::location */

#include "topos/location/manager.impl.h"

namespace vt {

extern location::LocationManager* theLocMan();

}  // end namespace vt

#endif /*INCLUDED_TOPOS_LOCATION_MANAGER_H*/
