
#if !defined INCLUDED_TOPOS_LOCATION_MANAGER_H
#define INCLUDED_TOPOS_LOCATION_MANAGER_H

#include "vt/config.h"
#include "vt/topos/location/location_common.h"
#include "vt/topos/location/location.fwd.h"
#include "vt/topos/location/manager.fwd.h"
#include "vt/topos/location/utility/coord.h"
#include "vt/vrt/vrt_common.h"

#include <unordered_map>
#include <functional>

namespace vt { namespace location {

struct LocationManager {
  template <typename LocType>
  using PtrType = std::unique_ptr<LocType>;
  using LocCoordPtrType = LocationCoord*;
  using LocInstContainerType = std::unordered_map<LocInstType, LocCoordPtrType>;
  using VrtLocType = EntityLocationCoord<int32_t>;
  using VrtLocProxyType = EntityLocationCoord<VirtualProxyType>;

  template <typename ColT, typename IndexT>
  using CollectionProxyType = ::vt::vrt::VirtualElmProxyType<ColT, IndexT>;
  template <typename ColT, typename IndexT>
  using VrtColl = EntityLocationCoord<CollectionProxyType<ColT, IndexT>>;
  template <typename ColT, typename IndexT>
  using CollectionLocType = PtrType<VrtColl<ColT, IndexT>>;
  using LocErasureType = LocationCoord;
  using LocDeleterType = std::function<void(LocErasureType*)>;
  using CollectionLocErasedType = std::unique_ptr<
    LocErasureType, LocDeleterType
  >;
  using CollectionContainerType = std::unordered_map<
    VirtualProxyType, CollectionLocErasedType
  >;
  template <typename LocType>
  using ActionLocInstType = std::function<void(LocType*)>;
  template <typename LocType>
  using PendingContainerType = std::vector<ActionLocInstType<LocType>>;

  LocationManager() = default;

  virtual ~LocationManager();

  static LocInstType cur_loc_inst;

  PtrType<VrtLocType> virtual_loc = std::make_unique<VrtLocType>();;
  PtrType<VrtLocProxyType> vrtContextLoc = std::make_unique<VrtLocProxyType>();

  template <typename ColT, typename IndexT>
  VrtColl<ColT, IndexT>* getCollectionLM(VirtualProxyType const& proxy);
  template <typename ColT, typename IndexT>
  void insertCollectionLM(VirtualProxyType const& proxy);

public:
  // Manage different instances of individually managed entities
  template <typename LocType>
  static void insertInstance(LocInstType const i, LocType* ptr);
  static LocCoordPtrType getInstance(LocInstType const inst);

  template <typename LocType>
  static void applyInstance(LocInstType const inst, ActionLocInstType<LocType> action);

  template <typename LocType>
  static std::unordered_map<
    LocInstType, PendingContainerType<LocType>
  > pending_inst_;

protected:
  CollectionContainerType collectionLoc;

 private:
  static LocInstContainerType loc_insts;
};

}} /* end namespace vt::location */

#include "vt/topos/location/manager.impl.h"

namespace vt {

extern location::LocationManager* theLocMan();

}  // end namespace vt

#endif /*INCLUDED_TOPOS_LOCATION_MANAGER_H*/
